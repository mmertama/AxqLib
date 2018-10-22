#ifndef REACTQUTION_PRIVATE_H
#define REACTQUTION_PRIVATE_H

#include <functional>
#include <QTimer>
#include <QVariant>
#include <QMetaMethod>
#include <QEvent>
#include <QSet>

#define PADDING4 const int s_padding = 0;

namespace Axq {

inline QVariant getParam(int){
    Q_ASSERT(false);
    return QVariant();
}

inline QVariant getParam(int index, const QVariant& i){
    Q_ASSERT(index == 0);
    return i;
}

template <typename ...Args>
QVariant getParam(int index, const QVariant& ii, Args...args){
    if(index == 0)
        return getParam(--index, ii);
    else
        return getParam(--index, args...);
}


/*Unfortunate copy-paste...*/
template <typename T>
QVariant convertFrom(const T& val){
    return QVariant::fromValue<T>(val);
}

template <typename T>
QVariant convertFrom(const QHash<QString, T>& val){
    QVariantHash hash;
    for(auto it = val.begin(); it != val.end(); ++it)
        hash.insert(it.key(), QVariant::fromValue<T>(it.value()));
    return QVariant::fromValue<QVariantHash>(hash);
}

template <typename T>
QVariant convertFrom(const QMap<QString, T>& val){
    QVariantMap map;
    for(auto it = val.begin(); it != val.end(); ++it)
        map.insert(it.key(), QVariant::fromValue<T>(it.value()));
    return QVariant::fromValue<QVariantMap>(map);
}

template <typename T>
QVariant convertFrom(const QList<T>& val){
    QVariantList lst;
    std::transform(val.begin(), val.end(), lst.begin(), [](const T& v){return  QVariant::fromValue<T>(v); });
    return QVariant::fromValue<QVariantList>(lst);
}


template <typename K, typename V> static void convertTo(QHash<K, V>& hash,  const QVariant& val){
    const auto iterable = val.toHash();
    for(auto i = iterable.constBegin(); i != iterable.constEnd(); i++){
        hash.insert(i.key(), i.value().value<V>());
    }
}

template <typename K, typename V> static void convertTo(QMap<K, V>& map,  const QVariant& val){
    const auto iterable = val.toMap();
    for(auto i = iterable.constBegin(); i != iterable.constEnd(); i++){
        map.insert(i.key(), i.value().value<V>());
    }
}

template <typename V> static void convertTo(QList<V>& list,  const QVariant& val){
    const auto iterable = val.toList();
    for(const auto&i : iterable){
        list.append(i.value<V>());
    }
}

template <typename T> static void convertTo(T&,  const QVariant&) {
}

template <class ...Arg> static void convertTo(Arg... args){
    convertTo(args...);
}

template<typename T>
T convert(const QVariant& val){
    if(val.canConvert<T>())
        return val.value<T>();
    T item;
    convertTo(item, val);
    return item;
}

class ProducerBase;

class Error{
public:
    virtual ~Error();
    virtual int errorCode() const  = 0;
    virtual QVariant error() const = 0;
    virtual bool isFatal() const = 0;
};

class SimpleError : public Error {
public:
    SimpleError() {}
    SimpleError(const QVariant& error, int code, bool isFatal = false) : m_error(error),
        m_code(code), m_fatal(isFatal){
    }
    QVariant error() const Q_DECL_OVERRIDE;
    int errorCode() const  Q_DECL_OVERRIDE;
    bool isFatal() const Q_DECL_OVERRIDE;
    QVariant m_error;
    int m_code = 0;
    int m_fatal = false; //int for padding complaining
};



class StreamBase : public QObject {
  Q_OBJECT
public:
    using ErrorFunction = std::function<void (const Error&)>;
    StreamBase(std::nullptr_t);
    StreamBase(StreamBase* parent);
    StreamBase(QObject* parent);
    virtual ~StreamBase() Q_DECL_OVERRIDE;
    virtual ProducerBase* producer();
    virtual bool isAlias(const QObject* item) const;     //tell if item is this, or if this has some childs
    virtual bool wait() const;                                   //tell is this is pending and complete cannot requested yet, but this may happend soon and then waitOver should be emitted
    virtual void cancel();                                  //upon cancel, default does nothing
    bool hasParent() const {return m_parent; }
    ProducerBase* root();
    void addChildren(StreamBase*  stream);
    void removeChildren(StreamBase* stream);
    template<class T>
    const QList<T*>  children() const;
    /*if this dies before call, the call is not done*/
     void delayedCall(std::function<void()> f);
     /*if receiver dies before call, the call is not done*/
     static void delayedCall(QObject* receiver, std::function<void()> f);
protected:
    virtual void initConnections();
    virtual void connectFinished();
protected:
     void childEvent(QChildEvent *event) Q_DECL_OVERRIDE;
signals:
    void next(const QVariant& next);
    void error(const Error& error);
    void waitOver();
    void finished(ProducerBase* origin);
protected:
    StreamBase* m_parent = nullptr;
private:
    void connectBaseHandlers();
    template<class T> void getAllChildren(QList<T*>&) const;
    QSet<StreamBase*> m_children;
};


template<class T>
inline const QList<T*> StreamBase::children() const {
    QList<T*> lst;
    getAllChildren<T>(lst);
    return lst;
}

template<class T>
inline void StreamBase::getAllChildren(QList<T*>& lst) const {
    for(auto chld : m_children){
        if(qobject_cast<StreamBase*>(chld)){
            chld->getAllChildren(lst);
            auto s = qobject_cast<T*>(chld); //may have even non-sterambase*
            if(s)
                lst.append(s);
        }
    }
}



class ParentStream : public StreamBase {
    Q_OBJECT
public:
    ParentStream(StreamBase* parent) : StreamBase(parent){}
    ParentStream(QObject* parent) : StreamBase(parent){}
    ParentStream(std::nullptr_t) : StreamBase(nullptr){}
    virtual void appendChild(StreamBase* child) = 0;
};

}


Q_DECLARE_METATYPE(Axq::SimpleError)


#endif // REACTQUTION_PRIVATE_H
