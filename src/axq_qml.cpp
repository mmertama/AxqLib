#include <limits>
#include <QQmlEngine>
#include <QTimer>
//#include <QRegularExpression>

#include "axq.h"
#include <QJSValueIterator>
#include "axq_qml.h"
#include "axq_streams.h"
#include "axq_producer.h"
#include "axq_threads.h"



#include <QDebug>

//OperatorQML

namespace Axq {

template <class Op>
class MonadQML : public StreamQML {
public:
    template<typename ...Args>
    MonadQML(const QJSValue& caller, EnvQML* env, StreamQML* stream, Args...args) :
        StreamQML(env, stream),
        m_onCaller(caller),
        m_operator(new Op(makeCall(caller), stream->stream(), args...)) {
        Q_ASSERT(stream->stream());
    }
    StreamBase* stream() {
        return m_operator;
    }
    operator QVariant() {return QVariant::fromValue<MonadQML<Op>*>(this);}
private:
    std::function<QVariant(const QVariant& v)> makeCall(const QJSValue& caller);
private:
    QJSValue m_onCaller;
    Op* m_operator;
};

template <class Op>
std::function<QVariant(const QVariant& v)> MonadQML<Op>::makeCall(const QJSValue& caller) {
    if(caller.isCallable())
        return [this](const QVariant & value)->QVariant{
        const auto param = m_env->js().toScriptValue<QVariant>(value);
        const auto v = m_onCaller.call({param}).toVariant();
        return v;
    };
    else return [](const QVariant & v)->QVariant{
        return v;
    };
}

template <class Op>
class DyadQML : public StreamQML {
public:
    template<typename ...Args>
    DyadQML(const QJSValue& caller, EnvQML* env, StreamQML* stream, Args...args) :
        StreamQML(env, stream),
        m_onCaller(caller),
        m_operator(new Op([this](const QVariant & value1, const QVariant & value2)->QVariant{
        const auto param1 = m_env->js().toScriptValue<QVariant>(value1);
        const auto param2 = m_env->js().toScriptValue<QVariant>(value2);
        const auto v = m_onCaller.call({param1, param2}).toVariant();
        return v;
    }, stream->stream(), args...)) {
        Q_ASSERT(stream->stream());
        //Q_ASSERT(caller.isCallable());
    }
    StreamBase* stream() {
        return m_operator;
    }
    operator QVariant() {return QVariant::fromValue<DyadQML<Op>*>(this);}
private:
    QJSValue m_onCaller;
    Op* m_operator;
};

}

using namespace Axq;

void Axq::registerTypes() {
    qmlRegisterSingletonType<Axq::EnvQML>("Axq", 1, 0, "Axq",
    [](QQmlEngine * qml, QJSEngine * js)->QObject* {
        return new Axq::EnvQML(qml, js);

    });
}



//EnvQML

EnvQML::EnvQML(QQmlEngine* qml, QJSEngine* js, QObject* parent) : QObject(parent), m_qml(qml), m_js(js) {}

QVariant EnvQML::range(const QVariant& begin, const QVariant& end, const QVariant& step) {
    const auto endValue = end.isValid() ? end.toInt() : std::numeric_limits<int>::max();
    const auto stepValue = step.isValid() ? step.toInt() : 1;
    return QVariant::fromValue<RangeQML*>(new RangeQML(begin.toInt(), endValue, stepValue, this, nullptr));
}


QVariant EnvQML::from(const QVariant& item) {
    return QVariant::fromValue<ContainerQML*>(new ContainerQML(item, this, nullptr));
}

QVariant EnvQML::makeError(const QString& errorString) {
    emit error(errorString);
    return QVariant::fromValue<NoneQML*>(new NoneQML(this, nullptr));
}


QVariant EnvQML::queue() {
    return QVariant::fromValue<QueueQML*>(new QueueQML(this, nullptr));
}

QVariant EnvQML::repeater(const QJSValue& caller, int ms) {
    return QVariant::fromValue<RepeaterQML*>(new RepeaterQML(caller, ms, this, nullptr));
}


QVariant EnvQML::merge(const QJSValueList& sources) {
    return QVariant::fromValue<MergeQML*>(new MergeQML(sources, this, nullptr));
}

QVariant EnvQML::iterator(const QJSValue& object) {
    return QVariant::fromValue<IteratorQML*>(new IteratorQML(object, this, nullptr));
}


//StreamQML

StreamQML::StreamQML(EnvQML* env, StreamQML* parent) : QObject(parent), m_env(env), m_parent(parent) {
    m_env->qml().setObjectOwnership(this, QQmlEngine::CppOwnership);
}


StreamQML::~StreamQML() {
}

QVariant StreamQML::each(const QJSValue& caller) {
    const auto consumer = new MonadQML<Each>(caller, m_env, this);
    return *consumer;
}

QVariant StreamQML::map(const QJSValue& caller) {
    const auto consumer = new MonadQML<Map>(caller, m_env, this);
    return *consumer;
}

QVariant StreamQML::filter(const QJSValue& caller) {
    const auto consumer = new MonadQML<Filter>(caller, m_env, this);
    return *consumer;
}

QVariant StreamQML::split(const QJSValue& functionName, const QJSValue& p1, const QJSValue& p2) {
    const auto consumer = new SplitQML(functionName, p1, p2, m_env, this);
    return QVariant::fromValue<SplitQML*>(consumer);
}

QVariant StreamQML::completeEach(const QJSValue& caller) {
    const auto consumer = new MonadQML<CompleteFilter>(caller, m_env, this);
    return *consumer;
}

QVariant StreamQML::completeEach() {
    QJSValue dummy;
    const auto consumer = new MonadQML<CompleteFilter>(dummy, m_env, this);
    return *consumer;
}


QVariant StreamQML::scan(const QJSValue& begin, const QJSValue& caller) {
    const auto consumer = new ScanQML(begin, caller, m_env, this);
    return QVariant::fromValue<ScanQML*>(consumer);
}

QVariant StreamQML::meta(int info, const QJSValue& caller) {
    const auto consumer = new DyadQML<Information>(caller, m_env, this, QVariant::fromValue<int>(info));
    return *consumer;
}

QVariant StreamQML::iterate(const QJSValue& caller) {
    const auto consumer = new MonadQML<List>(caller, m_env, this);
    return *consumer;
}

QVariant StreamQML::buffer(const QJSValue& caller) {
    return QVariant::fromValue<BufferQML*>(new BufferQML(caller, m_env, this));
}

QVariant StreamQML::buffer() {
    return QVariant::fromValue<BufferQML*>(new BufferQML(QJSValue(), m_env, this));
}

QVariant StreamQML::defer() {
    auto s = stream();
    Q_ASSERT(s);
    auto p = s->producer();
    Q_ASSERT(p);
    p->defer();
    return QVariant::fromValue<StreamQML*>(this);
}

void StreamQML::cancel() {
    auto s = stream();
    Q_ASSERT(s);
    auto p = s->producer();
    Q_ASSERT(p);
    p->doCancel();
}

void StreamQML::complete() {
    auto s = stream();
    Q_ASSERT(s);
    auto p = s->producer();
    Q_ASSERT(p);
    p->cancel();
}

QVariant StreamQML::delay(int ms) {
    const auto consumer = new DelayQML(ms, m_env, this);
    return QVariant::fromValue<DelayQML*>(consumer);

}

void StreamQML::makeError(const QJSValue& error, int code, bool isFatal) {
    stream()->error(SimpleError(error.toVariant(), code, isFatal));
}

StreamQML* StreamQML::findProducer(StreamQML* t, ProducerBase* p) {
    if(!t) {
        return nullptr;
    }
    if(t->stream() == p) {
        return t;
    }
    return findProducer(t->m_parent, p);
}

WaitQML::WaitQML(const QJSValue& caller, EnvQML* env, StreamQML* parent) : StreamQML(env, parent),
    m_wait(new Axq::Each([caller, parent, this](const QVariant & value) {
    auto outter = parent->stream();
    auto op = outter->producer();
    op->defer();
    const auto param = m_env->js().toScriptValue<QVariant>(value);
    auto c = caller;
    Q_ASSERT_X(c.isCallable(), "QML: Wait", "Wait must have a kfunction parameter");
    auto obj = c.call({param}).toQObject();
    auto qmlStream = qobject_cast<StreamQML*>(obj);
    Q_ASSERT_X(qmlStream, "QML: Wait", "Does wait function return a stream?");
    auto inner = qmlStream->stream();
    auto ip = inner->producer();
    op->addChildren(ip);
    Q_ASSERT(inner);
    QObject::connect(ip, &ProducerBase::completed, [ip, op](ProducerBase * p) {
        if(ip == p) {
            op->requestAgain();
        }
    });

}, parent->stream())) {
}


StreamBase* WaitQML::stream() {
    return m_wait;
}

DelayQML::DelayQML(int ms, EnvQML* env, StreamQML* parent) : StreamQML(env, parent),
    m_delay(new Axq::Delay(parent->stream(), ms)) {
}

StreamBase* DelayQML::stream() {
    return m_delay;
}

QVariant StreamQML::wait(const QJSValue& caller) {
    return QVariant::fromValue<StreamQML*>(new WaitQML(caller, m_env, this));
}

QVariant StreamQML::onComplete(const QJSValue& caller) {
    stream()->root()->pushCompleteHandler([caller, this](ProducerBase * p) {
        auto stream = findProducer(this, p);
        const auto param = m_env->js().toScriptValue<QVariant>(QVariant::fromValue<StreamQML*>(stream));
        auto c = caller;
        c.call({param});
    });
    return QVariant::fromValue<StreamQML*>(this);
}

QVariant StreamQML::onError(const QJSValue& caller) {
    Q_ASSERT(caller.isCallable());
    QObject::connect(this, &StreamQML::error, [this, caller](const QString & errorReason) {
        if(caller.isCallable()) {
            const auto param = m_env->js().toScriptValue<QString>(errorReason);
            auto c = caller;
            c.call({QJSValue(errorReason)});
        }
    });
    return QVariant::fromValue<StreamQML*>(this);
}

QVariant StreamQML::request(int delay) {
    Q_ASSERT(m_parent);
    m_parent->request(delay);
    return QVariant::fromValue<StreamQML*>(this);
}

void StreamQML::push(const QVariant& value) {
    Q_ASSERT(m_parent);
    m_parent->push(value);
}


//ProducerQML

ProducerQML::ProducerQML(EnvQML* env, StreamQML* parent) : StreamQML(env, parent) {
    QTimer::singleShot(0, [this]() {
        QObject::connect(stream()->producer(), &ProducerBase::completed, this, [this](ProducerBase * org) {
            if(org == stream()->producer()) {
                emit org->finalized();
            }
        });
    });
}


QVariant ProducerQML::onCompleted(const QJSValue& caller) {
    Q_ASSERT(caller.isCallable());
    QObject::connect(this, &ProducerQML::completed, [caller]() {
        auto c = caller;
        c.call();
    });
    return QVariant::fromValue<StreamQML*>(this);
}


QVariant ProducerQML::request(int delay) {
    auto p = stream()->producer();
    Q_ASSERT(p);
    p->request(delay);
    return QVariant::fromValue<StreamQML*>(this);
}

//RangeQML

RangeQML::RangeQML(int begin, int end, int step, EnvQML* env, StreamQML* stream) :
    ProducerQML(env, stream), m_range(new Range<QObject*>(begin, end, step, this)) {
    QObject::connect(m_range, &ProducerBase::completed, this, &ProducerQML::completed);
}


StreamBase* RangeQML::stream() {
    return m_range;
}

static ProducerBase* fromVariant(const QVariant& item, QObject* parent) {
    const auto t = item.type();
    if(t == QVariant::Hash) {
        return new Container<QVariantHash, QObject*>(item.toHash(), parent);
    }
    if(t == QVariant::Map) {
        return new Container<QVariantMap, QObject*>(item.toMap(), parent);
    }
    if(item.canConvert<QList<QVariant>>()) {
        return new Container<QVariantList, QObject*>(item.value<QVariantList>(), parent);
    }
    return new Value<QObject*>(item, parent);
}

ContainerQML::ContainerQML(const QVariant& item, EnvQML* env, StreamQML* stream) :
    ProducerQML(env, stream), m_container(fromVariant(item, this)) {
    QObject::connect(m_container, &ProducerBase::completed, this, &ProducerQML::completed);
}


StreamBase* ContainerQML::stream() {
    return m_container;
}

NoneQML::NoneQML(EnvQML* env, StreamQML* stream) :
    StreamQML(env, stream), m_nothing(new None(stream->stream())) {
}

StreamBase* NoneQML::stream() {
    return m_nothing;
}


QueueQML::QueueQML(EnvQML* env, StreamQML* stream) :
    ProducerQML(env, stream), m_queue(new QueueProducer(this)) {
}

void QueueQML::push(const QVariant& variant) {
    m_queue->push(variant);
}

StreamBase* QueueQML::stream() {
    return m_queue;
}

BufferQML::BufferQML(const QJSValue& max, EnvQML* env, StreamQML* parent) : StreamQML(env, parent),
    m_buffer(new Buffer(max.toInt(), parent->stream())) {

}

StreamBase* BufferQML::stream() {
    return m_buffer;
}

//Bug in compiler? or using lambdas, but even the same captures - ternary operator wont compile
std::function<QVariant()> RepeaterQML::makeCall(const QJSValue& caller, RepeaterQML* r) {
    if(caller.isCallable())
        return [r]()->QVariant{
        return r->m_onCaller.call().toVariant();
    };
    else
        return [r]()->QVariant{
        return r->m_onCaller.toVariant();
    };
}

RepeaterQML::RepeaterQML(const QJSValue& caller, int ms, EnvQML* env, StreamQML* parent) : ProducerQML(env, parent),
    m_onCaller(caller), m_repeater(new Axq::Repeater(makeCall(caller, this), ms, parent->stream())) {
}

StreamBase* RepeaterQML::stream() {
    return m_repeater;
}

template <class T>
static QList<T*> mapTo(const QJSValueList& sources) {
    QList<T*> streams;
    for(const auto& s : sources) {
        const auto stream = s.toVariant().value<T*>();
        Q_ASSERT(stream);
        streams.append(stream);
    }
    return streams;
}


MergeQML::MergeQML(const QJSValueList& sources, EnvQML* env, StreamQML* parent) : ProducerQML(env, parent),
    m_merge(new Axq::Merge(std::move(mapTo<StreamBase>(sources)), parent->stream())) {
}

StreamBase* MergeQML::stream() {
    return m_merge;
}


IteratorQML::IteratorQML(const QJSValue& object, EnvQML* env, StreamQML* parent) : ProducerQML(env, parent) {
    if(object.isArray()) {
        const auto len = static_cast<unsigned>(object.property("length").toInt());
        auto it = std::shared_ptr<unsigned>(new unsigned(0));
        m_iterator = new Axq::Iterator<QObject*>([it, len]() {
            return *it < len;
        }, [it, object]() {
            return object.property((*it)++).toVariant();  //yes its tested
        }, nullptr, this);
    }
    if(object.isString()) {
        const auto string = std::shared_ptr<QString>(new QString(object.toString()));
        const auto it = std::shared_ptr<QString::const_iterator>
                        (new QString::const_iterator(string->begin()));

        m_iterator = new Axq::Iterator<QObject*>([it, string]() {
            return *it != string->end();
        }, [it, string]() {
            auto c =  QString(*(*it));
            ++(*it);
            return c;
        }, nullptr, this);
    } else {
        auto it = std::shared_ptr<QJSValueIterator>(new QJSValueIterator(object));
        m_iterator = new Axq::Iterator<QObject*>([it, object]() {
            return it->hasNext();
        }, [it]() {
            it->next();
            return it->value().toVariant();
        }, nullptr, this);
    }
}

StreamBase* IteratorQML::stream() {
    return m_iterator;
}

ScanQML::ScanQML(const QJSValue& begin, const QJSValue& caller, EnvQML* env, StreamQML* parent) :
    StreamQML(env, parent),
    m_acc(begin),
    m_caller(caller),
    m_scan(new Axq::Scan(
               [this]()->QVariant{
    return m_acc.toVariant();
},
[this](const QVariant & value) {
    const auto param2 = m_env->js().toScriptValue<QVariant>(value);
    m_acc = m_caller.call({m_acc, param2});
}, parent->stream())) {
}

StreamBase* ScanQML::stream() {
    return m_scan;
}


SplitQML::SplitQML(const QJSValue& functionName, const QJSValue& p1, const QJSValue& p2, EnvQML* env, StreamQML* parent) :
    StreamQML(env, parent),
    m_split(new Axq::Split(parent->stream())) {

    auto meta = m_parent->metaObject();
    const auto func = functionName.toString().toStdString().c_str();
    auto index = meta->methodCount() - 1;
    for(; index >= 0; index--) {
        auto m = meta->method(index);
        if(m.isValid()
                && m.access() == QMetaMethod::Public
                && m.name() == func) {
            break;
        }
    }
    Q_ASSERT_X(index >= 0, "QML calls invalid function", func);
    const auto method = metaObject()->method(index);
    QVariant returnValue;
    bool success = false;
    switch(method.parameterCount()) {
    case 0:
        success = method.invoke(m_parent,
                                Q_RETURN_ARG(QVariant, returnValue));
        break;
    case 1:
        success = method.invoke(m_parent,
                                Q_RETURN_ARG(QVariant, returnValue),
                                Q_ARG(QJSValue, p1));
        break;
    case 2:
        success = method.invoke(m_parent,
                                Q_RETURN_ARG(QVariant, returnValue),
                                Q_ARG(QJSValue, p1),
                                Q_ARG(QJSValue, p2));
        break;
    default:
        Q_ASSERT_X(false, "QML calls with too many paramters", func);
    }
    Q_ASSERT_X(success, "QML call failed", func);

    //API return child as variant
    auto childStream = returnValue.value<StreamQML*>();
    //and then just add stream
    m_split->appendChild(childStream->stream());
}

StreamBase* SplitQML::stream() {
    return m_split;
}
