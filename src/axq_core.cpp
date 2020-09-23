#include "axq.h"
#include "axq_producer.h"
#include "axq_operators.h"
#include "axq_threads.h"
#include "axq_private.h"


using namespace Axq;

Stream::Keeper::~Keeper() {}
Stream::Waiter::~Waiter() {}

constexpr bool equal(char const* a, char const* b) {
    return *a == *b && (*a == '\0' || equal(a + 1, b + 1));
}

// Certain values are copied as I wanna keep axq.h not visible into StreamBase implementations
//(to keep QML API tidy), therefore copied integrity is check upon compile time here.
static_assert(equal(Axq::Stream::StreamCancel, Axq::StreamCancel), "mismatch");
static_assert(Axq::Stream::StreamCancelValue == Axq::StreamCancelValue, "mismatch");
static_assert(Axq::Stream::RequestOne == Axq::RequestOne, "mismatch");

Stream::Stream() : Stream(nullptr) {
}

Stream::Stream(ProducerBase* ptr) : m_ptr(ptr) { /*, m_private(ptr)*/
    if(ptr && !ptr->parent()) {
        Q_ASSERT(!m_private);
        m_private.reset(ptr);
        new StreamPrivate(m_private);
    }
}

Stream::Stream(StreamBase* ptr, const Stream& parent) : m_ptr(ptr), m_private(parent.m_private) {
}

Stream::Stream(ProducerBase* producer, std::nullptr_t) : m_ptr(producer) {

}

Stream Stream::onError(std::function <void (const QVariant& error, int code)> handler) {
    stream()->producer()->setErrorHandler([handler](const Error & e) {
        handler(e.error(), e.errorCode());
    });
    return *this;
}

Stream Stream::onCompleted(std::function <void (const Stream&)> handler) {
    auto current = stream()->root();
    auto ptr = m_private;
    current->pushCompleteHandler([handler, ptr](ProducerBase*) {
        auto* pp = qobject_cast<ProducerBase*>(ptr.get());
        handler(Stream(pp, nullptr)); //already dead producer
    });
    return *this;
}

Stream Stream::onCompleted(std::function <void ()> handler) {
    stream()->root()->pushCompleteHandler([handler](ProducerBase*) {
        handler();
    });
    return *this;
}

Stream Stream::createOnCompleted(std::function<void (const QVariant& val)> last) {
    auto* v = new QVariant();
    own(v);
    auto each = createEach([v](const QVariant & val) {
        *v = val;
    });
    stream()->root()->pushCompleteHandler([last, v](ProducerBase*) {
        last(*v);
    });
    return each;
}


Stream Stream::createEach(std::function<void (const QVariant&)> f) {
    Q_ASSERT(f);
    return Stream(new Each(f, stream()), *this);
}

Stream Stream::iterate() {
    return Stream(new List(nullptr, stream()), *this);
}

Stream Stream::createBuffer(int max) {
    return Stream(new Buffer(max, stream()), *this);
}

Stream Stream::createCompleteFilter(std::function<bool (const QVariant&)> f) {
    return Stream(new CompleteFilter(f, stream()), *this);
}

Stream Stream::createMap(std::function<QVariant(const QVariant&)> f) {
    Q_ASSERT(f);
    return Stream(new Map(f, stream()), *this);
}

Stream Stream::createSpawn(std::function<Stream(const QVariant&)> f) {
    Q_ASSERT(f);
    auto outter = stream();
    return Stream(new Each([f, outter](const QVariant & v) {
        auto op = outter->producer();
        op->defer();
        auto inner = f(v).stream();
        auto ip = inner->producer();
        op->addChildren(ip);
        Q_ASSERT(inner);
        QObject::connect(ip, &QObject::destroyed, op, [op]() {
            op->requestAgain();
        });
        QObject::connect(ip, &ProducerBase::completed, op, [ip, op](ProducerBase * p) {
            if(ip == p) {
                op->requestAgain();
            }
        });
    }, stream()), *this);
}

std::tuple<Stream, Stream> Stream::createAsync() {
    auto watcher = new AsyncWatcher(stream());
    return std::make_tuple(Stream(watcher, *this), Stream(watcher->async(), *this));
}

std::tuple<Stream, Stream> Stream::createSplit() {
    auto split = new Split(stream());
    return std::make_tuple(Stream(split, *this), Stream(split->parent(), *this));
}


void Stream::setChild(const Stream& stream) { //in theory lowerlevel supports multiple ops in thread
    qobject_cast<ParentStream*>(m_ptr)->appendChild(stream.m_ptr);
}


Stream Stream::createFilter(std::function<bool
                            (const QVariant&)> f) {
    Q_ASSERT(f);
    return Stream(new Filter(f, stream()), *this);
}

Stream Stream::createScan(std::function<QVariant()> getAcc,
                          std::function<void (const QVariant&)> f) {
    Q_ASSERT(f);
    return Stream(new Scan(getAcc, f, stream()), *this);
}

static int to(Axq::Stream::InfoValues i) {
    switch(i) {
    case Axq::Stream::InfoValues::Index: return Information::Index;
    case Axq::Stream::InfoValues::This: break; //not supported
    }
    Q_ASSERT(false);
    return 0;
}

Stream Stream::createInfo(Axq::Stream::InfoValues infoType, std::function<QVariant(const QVariant&, const QVariant&)> f) {
    if(infoType == InfoValues::This) {
        auto producer = stream()->producer();
        return Stream(new Each([f, producer](const QVariant & val) {
            return f(val, QVariant::fromValue<Stream>(Stream(producer, nullptr)));
        }, stream()), *this);
    } else {
        return Stream(new Information(f, stream(), to(infoType)), *this);
    }
}

Stream Stream::createDelay(int delayMs) {
    Q_ASSERT(delayMs >= 0);
    return Stream(new Delay(stream(), delayMs), *this);
}

Stream Stream::createList(std::function<QVariant(const QVariant&)> f) {
    Q_ASSERT(f);
    return Stream(new List(f, stream()), *this);
}



Stream Stream::createOwner(Stream::Keeper* deleter) {
    new Owner(stream()->producer(), [deleter]() {delete deleter;});
    return *this;
}

Stream Stream::take(const Stream& other) {
    auto otherRoot = other.stream()->producer();
    for(auto& owner : otherRoot->findChildren<Owner*>()) { //owner is not StreamBase inherited
        owner->setParent(stream());
    }
    return *this;
}


StreamBase* Stream::stream() const {
    return m_ptr;
}

Stream Stream::createIterator(Keeper* iterator, std::function<bool ()> hasNext, std::function<QVariant()> next) {
    return  Stream(new Iterator<std::nullptr_t>(hasNext, next, [iterator]() {delete iterator;}, nullptr));
}

Stream Stream::createContainer(const QVariant& container) {
    if(container.canConvert<QVariantHash>()) {
        return Stream(new Container<QVariantHash, std::nullptr_t>(container.value<QVariantHash>(), nullptr));
    }
    if(container.canConvert<QVariantMap>()) {
        return Stream(new Container<QVariantMap, std::nullptr_t>(container.value<QVariantMap>(), nullptr));
    }
    if(container.canConvert<QVariantList>()) {
        return Stream(new Container<QVariantList, std::nullptr_t>(container.value<QVariantList>(), nullptr));
    }
    return Stream(new Value<std::nullptr_t>(container, nullptr));
}


QList<StreamBase*> Stream::mapToStream(const QList<Stream>& sources) {
    QList<StreamBase*> streams;
    for(const auto& s : sources) {
        streams.append(s.stream());
    }
    return streams;
}

QList<ProducerBase*> Stream::mapToProducer(const QList<Stream>& sources) {
    QSet<ProducerBase*> streams;
    for(const auto& s : sources) {
        streams.insert(s.stream()->producer());
    }
    return streams.toList();
}


Stream Stream::defer() {
    auto s = stream();
    Q_ASSERT(s);
    auto p = s->producer();
    Q_ASSERT(p);
    p->defer();
    return *this;
}

Stream Stream::request(int delayMs) {
    auto s = stream();
    Q_ASSERT(s);
    auto p = s->producer();
    Q_ASSERT(p);
    p->request(delayMs);
    return *this;
}

void Stream::cancel() {
    auto s = stream();
    //  qDebug() << "cancel" << s << m_private.use_count() << s->parent() ;
    Q_ASSERT(s);
    auto p = s->root();
    Q_ASSERT(p);
    p->doCancel();
}

Stream Stream::completeEach() {
    return createCompleteFilter([](const QVariant&)->bool {
        return true;
    });
}

void Stream::complete() {
    stream()->producer()->cancel();
}

Stream Stream::createRepeater(std::function<QVariant()> f, int intervalMs) {
    Q_ASSERT(f);
    Axq::ProducerBase* ptr = new Axq::Repeater(f, intervalMs, nullptr);
    return Stream(ptr);
}

Stream Stream::async() {
    auto producer = stream()->root();
    Q_ASSERT(producer);
    Axq::ProducerBase* ptr = new Axq::AsyncProducer(producer, m_private);
    return Stream(ptr);
}


Stream Axq::read(QIODevice* device, int len) {
    auto ptr = new Axq::FuncProducer(nullptr, nullptr);

    if(!device->parent()) {
        device->setParent(ptr);
    }

    if(!device->isOpen()) {
        device->open(QIODevice::ReadOnly);
    }
    if(!device->isOpen()) {
        ptr->delayedCall([ptr] {
            ptr->error(SimpleError("Cannot open", -1));
        });
        return Stream(ptr);
    }

    std::function<QByteArray()> rf = nullptr;

    if(len == Stream::Slurp) {
        rf = [device]() {return device->readAll();};
    } else if(len == Stream::OneLine) {
        rf = [device]() {return device->readLine();};
    } else
        rf = [device, len]() {return device->read(len);};

    ptr->set([device, ptr, rf]()->QVariant{
        if(!device->atEnd()) {
            ptr->request(0); //why virtual function is not found (gcc) ?
            return rf();
        } else {
            ptr->complete();
            return QVariant();
        }
    });
    return Stream(ptr);
}

Stream Stream::create(std::function<QVariant()> function) {
    Q_ASSERT(function);
    ProducerBase* ptr = new Axq::FuncProducer(function, nullptr);
    return Stream(ptr);
}

Stream Axq::range(int begin, int end, int step) {
    Axq::ProducerBase* ptr = new Axq::Range<std::nullptr_t>(begin, end, step, nullptr);
    return Stream(ptr);
}

Stream Axq::from(QObject* object) {
    Axq::ProducerBase* ptr = new Axq::Value<std::nullptr_t>(QVariant::fromValue(object), nullptr);
    if(!object->parent()) {
        object->setParent(ptr);
    }
    return Stream(ptr);
}

Stream Axq::merge(const QList<Stream>& streams) {
    Axq::ProducerBase* ptr = new Axq::Merge(Stream::mapToStream(streams), nullptr);
    return Stream(ptr);
}

Stream Axq::create(Queue* push) {
    auto ptr = new Axq::QueueProducer(nullptr);
    if(!push->parent()) {
        push->setParent(ptr);
    }
    QObject::connect(push, static_cast<void (Queue::*)(const QVariant&) >(&Queue::push), ptr, &QueueProducer::push);
    QObject::connect(push, &Queue::complete, ptr, &QueueProducer::doComplete);
    return Stream(ptr);
}

Queue::Queue(QObject* parent) : QObject(parent) {
}

void Stream::makeError(const QVariant& errorData, int code, bool isFatal) {
    stream()->error(SimpleError(errorData, code, isFatal));
}

std::tuple<Stream, Stream::Waiter*> Stream::createWait() {
    auto wait = new Wait(stream());
    class WaiterImp : public Waiter {
    public:
        WaiterImp(Wait* w) : m_wait(w) {}
        ~WaiterImp() {
            if(m_wait)
                m_wait->unwait();
            if(m_connection) {
                 QObject::disconnect(m_connection);
            }
        }
        void finished() override {
            if(m_wait) {
                m_wait->unwait();
                m_wait = nullptr;
            }
            if(m_connection)
                QObject::disconnect(m_connection);
        }
        void connected(const QMetaObject::Connection& conn) override {
            Q_ASSERT(!m_connection);
            m_connection = conn;
        }
    private:
        Wait* m_wait = nullptr;
        QMetaObject::Connection m_connection;
    };
    auto wo = new WaiterImp(wait);
    own(wo);
    return std::tuple<Stream, Stream::Waiter*>(Stream(wait, *this), wo);
}

void Stream::applyParent(QObject* obj) {
    auto owner = new Owner(stream()->producer(), []() {});
    obj->setParent(owner);
}


