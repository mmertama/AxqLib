#ifndef AXQ_OPERATORS_H
#define AXQ_OPERATORS_H

#include "axq_producer.h"

namespace Axq {

class Operator : public StreamBase {
    Q_OBJECT
public:
    template<typename ...Args>
    Operator(StreamBase* parent) :  StreamBase(parent) {}
    void initConnections() Q_DECL_OVERRIDE;
};

/*
 * I wish I could use templated parameters instead of Variants, but those cannot be signalled
 * and as wholy async loop thing is based on signals that wont work that well
*/
class Map : public Operator {
    Q_OBJECT
public:
    template<typename ...Args>
    Map(std::function<QVariant(const QVariant&)> map, StreamBase* parent) : Operator(parent) {
        //"this" in slot is very important, it tells that slot is executed in this-object thread instead of constructor time thread, which may differ
        QObject::connect(m_parent, &StreamBase::next, this, [map, this](const QVariant & value) {
            const auto v = map(value);
            emit next(v);
        });
    }
};

class Filter : public Operator {
    Q_OBJECT
public:
    template<typename ...Args>
    Filter(std::function<QVariant(const QVariant&)> filter, StreamBase* parent)  : Operator(parent) {
        QObject::connect(m_parent, &StreamBase::next,  this, [this, filter](const QVariant & value) {
            const auto v = filter(value);
            if(v.isValid() && v.toBool()) {
                emit next(value);
            }
        });
    }
};



class Information : public Operator {
    Q_OBJECT
public:
    enum {
        Index = 1
    };
    template<typename ...Args>
    Information(std::function<QVariant(const QVariant&, const QVariant& v)> f, StreamBase* parent, Args... args)  : Operator(parent),
        m_type(getParam(0, args...).toInt()) {
        switch(m_type) {
        case Index:
            QObject::connect(m_parent, &StreamBase::next,  this, [this, f](const QVariant & value) {
                emit next(f(value, QVariant::fromValue<ulong>(m_count)));
                ++m_count;
            });
            break;
        default:
            Q_ASSERT(false);
        }
    }
    int m_type = 0;
    ulong m_count = 0;
};


class Delay : public Operator {
    Q_OBJECT
public:
    Delay(StreamBase* parent, int ms);
    bool wait() const Q_DECL_OVERRIDE;
    void cancel() Q_DECL_OVERRIDE;
protected:
    void initConnections() Q_DECL_OVERRIDE;
private:
    QTimer* get(int delay);
private:
    int m_onWait = 0;
    ProducerBase* m_finished = nullptr;
    QList<QTimer*> m_timerPool;
};


class Scan : public Operator {
    Q_OBJECT
public:
    Scan(std::function<QVariant()> getAcc, std::function<void (const QVariant& variant)> scan, StreamBase* parent);
    bool wait() const Q_DECL_OVERRIDE;
    void cancel() Q_DECL_OVERRIDE;
private:
    bool m_pending = false;
};


class Each : public Operator {
    Q_OBJECT
public:
    template<typename ...Args>
    Each(std::function<void (const QVariant& variant)> each, StreamBase* parent) : Operator(parent) {
        QObject::connect(m_parent, &StreamBase::next, this, [this, each](const QVariant & value) {
            each(value);
            emit next(value);
        });
    }
};


class CompleteFilter : public Operator {
    Q_OBJECT
public:
    template<typename ...Args>
    CompleteFilter(std::function<QVariant(const QVariant&)> filter, StreamBase* parent)  : Operator(parent) {
        QObject::connect(m_parent, &StreamBase::next,  this, [this, filter](const QVariant & value) {
            const auto v = filter(value);
            if(v.isValid() && v.toBool()) {
                producer()->complete();
            } else {
                emit next(value);
            }
        });
    }
};

class Buffer : public Operator {
    Q_OBJECT
public:
    Buffer(int max, StreamBase* parent);
    void cancel() Q_DECL_OVERRIDE;
private:
    QVariantList m_buffer;
};


class Split : public ParentStream {
    Q_OBJECT
public:
    Split(StreamBase* parent);
    void appendChild(StreamBase* child) Q_DECL_OVERRIDE;
    StreamBase* parent();
    void cancel() Q_DECL_OVERRIDE;
private:
    void tryNext();
private:
    StreamBase* m_child = nullptr;
    QVariantList m_nexts;
    int m_emits = 0;
};


class Wait : public Operator {
    Q_OBJECT
public:
    Wait(StreamBase* parent);
    bool wait() const Q_DECL_OVERRIDE;
    void unwait();
private:
    bool m_wait = true;
};

}

#endif // AXQ_OPERATORS_H
