#ifndef AXQ_CORE_H
#define AXQ_CORE_H

#include "axq_streams.h"

namespace Axq {

constexpr int RequestOne = -1;
constexpr int DoDefer = -2;

constexpr char StreamCancel[] = "Stream::Cancel";
constexpr int StreamCancelValue = 1000;

class ProducerBase : public StreamBase {
    Q_OBJECT
public:
    using CompleteFunction = std::function<void (ProducerBase*)>;
    ProducerBase(StreamBase* parent);
    ProducerBase(QObject* parent);
    ProducerBase(std::nullptr_t);
    virtual void request(int milliseconds);
    virtual void request();
    virtual void defer();
    ProducerBase* producer() Q_DECL_OVERRIDE;
    bool wait() const Q_DECL_OVERRIDE = 0;
    void setErrorHandler(ErrorFunction f);
    void pushCompleteHandler(CompleteFunction f);
    ~ProducerBase() Q_DECL_OVERRIDE;
     void requestAgain();
     void doCancel();
signals:
    void completed(ProducerBase* origin);
    void finalized();
public slots:
    virtual void complete();    //graceful end
    virtual void cancel() Q_DECL_OVERRIDE;      //suspending end
private slots:
    void atEnd();
    void atError(const Error& err);
protected:
    void initConnections() Q_DECL_OVERRIDE;
private:
    void cancelAll();
protected:
    QList<CompleteFunction> m_completeHandler;
    ErrorFunction m_errorHandler =  nullptr;
    int m_lastRequest = 0;
};


class QueueProducer : public ProducerBase {
    Q_OBJECT
public:
    QueueProducer(StreamBase* parent);
    QueueProducer(QObject* parent);
    QueueProducer(std::nullptr_t);
    bool wait() const Q_DECL_OVERRIDE;
private:
    void init();
signals:
    void push(const QVariant& value);
    void doComplete();
};

class Serializer : public ProducerBase {
    Q_OBJECT
public:
    Serializer(StreamBase* parent) : ProducerBase(parent){init();}
    Serializer(QObject* parent) : ProducerBase(parent){init();}
    Serializer(std::nullptr_t) : ProducerBase(nullptr){init();}
    ~Serializer() Q_DECL_OVERRIDE;
    void complete() Q_DECL_OVERRIDE;
    void request(int milliseconds) Q_DECL_OVERRIDE;
    void defer() Q_DECL_OVERRIDE;
    virtual bool hasData() const = 0;
    bool wait() const Q_DECL_OVERRIDE;
    void request() Q_DECL_OVERRIDE;
    void cancel() Q_DECL_OVERRIDE = 0;
signals:
    void requestOne();
    void dataAdded();
private:
    void init();
protected:
    virtual void onNext() = 0;
private:
    QTimer* m_timer = nullptr;
    int m_delay = 0;
};



class None : public ProducerBase {
    Q_OBJECT
public:
    None(StreamBase* parent);
    bool wait() const Q_DECL_OVERRIDE;
};


class FuncProducer : public Serializer {
    Q_OBJECT
public:
    FuncProducer(std::function<QVariant ()> function, std::nullptr_t parent);
    FuncProducer(std::function<QVariant ()> function, QObject* parent);
    FuncProducer(std::function<QVariant ()> function, StreamBase* parent);
    bool hasData() const Q_DECL_OVERRIDE;
    void request(int ms) Q_DECL_OVERRIDE;
    void set(std::function<QVariant ()> function);
    void cancel() Q_DECL_OVERRIDE;
private:
    void onNext() Q_DECL_OVERRIDE;
private:
    std::function<QVariant ()> m_f = nullptr;
    bool m_called = false;
};

template <typename PARENT = StreamBase*>
class Range : public Serializer {
public:
    Range(int begin, int end, int step, PARENT parent);
    bool hasData() const Q_DECL_OVERRIDE {
        return m_acc < m_end;
    }
    void cancel() Q_DECL_OVERRIDE {
        m_acc = m_end;
        ProducerBase::cancel();
    }
protected:
    void onNext() Q_DECL_OVERRIDE;
private:
    int m_acc;
    const int m_end;
    const int m_step;
};


//Range

template <typename PARENT>
Range<PARENT>::Range(int begin, int end, int step, PARENT parent) :
    Serializer(parent), m_acc(begin), m_end(end), m_step(step){
    delayedCall([this](){
        if(!hasData()){
            complete();
        }
    });
}

template <typename PARENT>
void Range<PARENT>::onNext(){
    if(m_acc < m_end){
        emit this->next(m_acc);
        m_acc += m_step;
    }
}


template <typename CONTAINER, typename PARENT = StreamBase*>
class Container : public Serializer {
public:
   Container(PARENT parent) : Serializer(parent){
       m_it = m_container.constBegin();
   }
   Container(CONTAINER&& container, PARENT parent) : Serializer(parent), m_container(container), m_it(container.constBegin()){}
   Container(const CONTAINER& container, PARENT parent) : Serializer(parent), m_container(container), m_it(container.constBegin()){}
   void set(CONTAINER&& c) {
       m_container = c;
       m_it = m_container.constBegin();
   }

   bool hasData() const Q_DECL_OVERRIDE {
       return !m_container.isEmpty();
   }
   void append(const QVariant& item){
       const auto d = std::distance(m_container.constBegin(), m_it);
       //we dunno if realloc happen, right?
       m_container.append(item);
       m_it = m_container.constBegin();
       std::advance(m_it, d);
   }

   void cancel() Q_DECL_OVERRIDE {
       m_container.clear();
       ProducerBase::cancel();
   }

 protected:
    void onNext() Q_DECL_OVERRIDE {
        if(m_it != m_container.constEnd()){
            emit this->next(*m_it);
            ++m_it;
        } else if(!m_container.isEmpty()) { //nothing there in the beginning
            m_container.clear();
            m_it = m_container.constBegin();
            this->complete();
        }
    }
private:
    CONTAINER m_container;
    typename CONTAINER::const_iterator m_it;
};


template <class PARENT = StreamBase*>
class Value : public Serializer {
public:
    Value(const QVariant& value, PARENT parent);
    bool hasData() const Q_DECL_OVERRIDE {return m_value.isValid();}
    void cancel() Q_DECL_OVERRIDE {
        m_value = QVariant();
        ProducerBase::cancel();
    }
protected:
    void onNext() Q_DECL_OVERRIDE;
private:
    QVariant m_value;
};


template <class PARENT>
Value<PARENT>::Value(const QVariant& value, PARENT parent) :
    Serializer(parent), m_value(value){
    Q_ASSERT(value.isValid());
    if(value.canConvert<QObject*>()){
        auto qo = value.value<QObject*>();
        QObject::connect(qo, &QObject::destroyed, this, [this](){
            m_value = QVariant(); //invalidate on destroy;
        });
    }
}

template <class PARENT>
void Value<PARENT>::onNext(){
    if(m_value.isValid()){
        emit this->next(m_value);
        m_value = QVariant();
    }
}


class Repeater : public ProducerBase {
    Q_OBJECT
public:
    using RepeatFunction = std::function<QVariant ()>;
    Repeater(RepeatFunction f, int ms, StreamBase* parent);
    Repeater(RepeatFunction f, int ms, QObject* parent);
    Repeater(RepeatFunction f, int ms, std::nullptr_t);

    void complete() Q_DECL_OVERRIDE;
    void request(int milliSeconds) Q_DECL_OVERRIDE;
    void defer() Q_DECL_OVERRIDE;
    bool wait() const Q_DECL_OVERRIDE;
    void cancel() Q_DECL_OVERRIDE;
signals:
    void requestOne();
private:
    void init(RepeatFunction f, int ms);
private:
    QTimer* m_timer;
    int m_delay = 0;
    PADDING4
};

class List : public ProducerBase {
    Q_OBJECT
public:
    List(std::function<QVariant (const QVariant&)> concat, StreamBase* parent);
    bool isAlias(const QObject* item) const Q_DECL_OVERRIDE;
    ProducerBase* producer() Q_DECL_OVERRIDE;
    bool wait() const Q_DECL_OVERRIDE;
    void cancel() Q_DECL_OVERRIDE;
private:
    void initConnections() Q_DECL_OVERRIDE {} // not using default handlers
private:
    Container<QVariantList>* m_container;
    bool m_pending = true;
};

class Merge : public ProducerBase {
    Q_OBJECT
public:
   //take parentship of sources
    Merge(const QList<StreamBase*>& sources, QObject* parent);
    ProducerBase* producer() Q_DECL_OVERRIDE;
    void complete() Q_DECL_OVERRIDE;
    bool wait() const Q_DECL_OVERRIDE;
    void cancel() Q_DECL_OVERRIDE;
private:
    QSet<StreamBase*> m_sources;
};

template <class PARENT = StreamBase*>
class Iterator : public Serializer {
public:
    Iterator(std::function<bool ()> hasNext, std::function<QVariant ()> next, std::function<void()> onEnd, PARENT parent)
        : Serializer(parent), m_hasNext(hasNext), m_next(next), m_onEnd(onEnd){}
    ~Iterator() Q_DECL_OVERRIDE {
        m_onEnd();
    }
protected:
    void onNext() Q_DECL_OVERRIDE {
        if(m_hasNext()){
            emit this->next(m_next());
        } else {
            emit waitOver();
        }
    }
    bool hasData() const Q_DECL_OVERRIDE {
        return m_hasNext && m_hasNext();
    }
    void cancel() Q_DECL_OVERRIDE {
        m_hasNext = nullptr;
        ProducerBase::cancel();
    }
private:
    std::function<bool ()> m_hasNext;
    std::function<QVariant ()> m_next;
    std::function<void ()> m_onEnd;
};


}

#endif // AXQ_CORE_H
