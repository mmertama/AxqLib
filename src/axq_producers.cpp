#include <QThread>
#include <QAbstractEventDispatcher>
#include "axq_producer.h"

#if defined(MEASURE_TIME) || defined(MEASURE_MEM)
#include "axq.h"
#elif !defined(ELAPSED)
#define ELAPSED(n)
#endif

using namespace Axq;

//Producer
ProducerBase* ProducerBase::producer() {
    return this;
}


// ProducerPrivate

ProducerBase::ProducerBase(StreamBase* parent) : StreamBase(parent) {
}

ProducerBase::ProducerBase(QObject* parent) : StreamBase(parent) {
}


ProducerBase::ProducerBase(std::nullptr_t) : StreamBase(nullptr) {
}


void ProducerBase::atEnd() {
    while(!m_completeHandler.isEmpty()) {
        m_completeHandler.takeFirst()(this);
    }

}

void ProducerBase::atError(const Error& err) {
    if(m_errorHandler) {
        m_errorHandler(err);
    }
}

void ProducerBase::initConnections() {
    connectFinished();
    QObject::connect(this, &ProducerBase::completed, this, [this](ProducerBase * origin) {
        Q_UNUSED(origin);
        emit waitOver();
        atEnd();

    }, Qt::QueuedConnection);

    QObject::connect(this, &ProducerBase::finalized, this, [this]() {
        emit waitOver();
        atEnd();
    }, Qt::QueuedConnection);

    QObject::connect(this, &StreamBase::error, this, [this](const Error & err) {
        for(auto c : root()->children<ProducerBase>()) {
            c->atError(err);
        }
        root()->atError(err);
        if(err.isFatal()) {
            cancelAll();
        }
    });
}

void ProducerBase::cancelAll() {
    for(const auto& chld : root()->children<ProducerBase>()) {
        chld->m_completeHandler.clear(); //cancelled wont complete
    }
    m_completeHandler.clear();
    root()->cancel();
}

void ProducerBase::requestAgain() {
    request(m_lastRequest);
}

void ProducerBase::request(int r) {
    m_lastRequest = r;
}

void ProducerBase::request() {
    request(0);
}

void ProducerBase::defer() {
}

void ProducerBase::pushCompleteHandler(CompleteFunction f) {
    if(f) {
        m_completeHandler.append(f);
    }
}


//efficitively should KILL this, and therefore cancel all timers etc.
void ProducerBase::cancel() {
    for(const auto& chld : children<StreamBase>()) {
        chld->cancel();
    }
    emit completed(this);
}

void ProducerBase::doCancel() {
    cancelAll();
    error(SimpleError(StreamCancel, StreamCancelValue));
}

//All subclasses should call this, instead of direct emit
void ProducerBase::complete() {
    emit finished(this);

    for(const auto& chld : children<StreamBase>()) {
        if(chld->wait()) {
            const bool ok = QObject::connect(chld, &StreamBase::waitOver, this, &ProducerBase::complete, Qt::QueuedConnection);
            Q_ASSERT(ok);
            return; // if one chold is busy we try again later
        }
    }
    emit completed(this);
}



void ProducerBase::setErrorHandler(ErrorFunction f) {
    m_errorHandler = f;
}

ProducerBase::~ProducerBase() {
    Q_ASSERT(m_completeHandler.isEmpty());
}


QueueProducer::QueueProducer(StreamBase* parent) : ProducerBase(parent) {init();}
QueueProducer::QueueProducer(QObject* parent) : ProducerBase(parent) {init();}
QueueProducer::QueueProducer(std::nullptr_t) : ProducerBase(nullptr) {init();}
void QueueProducer::init() {
    QObject::connect(this, &QueueProducer::push, this, &QueueProducer::next);
    QObject::connect(this, &QueueProducer::doComplete, this, [this]() {
        complete();
    });
}

bool QueueProducer::wait() const {
    return false; //queue's has nothing to wait, right?
}

//None
None::None(StreamBase* parent) : ProducerBase(parent) {
}

bool None::wait() const {
    return false;
}

static QVariantList makeList(const QVariant& value) {
    const auto list = convert<QVariantList>(value);
    if(!list.isEmpty()) {
        return list;
    }
    const auto map = convert<QVariantMap>(value);
    if(!map.isEmpty()) {
        QVariantList lst;
        lst.reserve(map.size());
        for(auto it = map.begin(); it != map.end(); ++it)
            lst.append(QVariant::fromValue(QVariantList({it.key(), it.value()})));
        return lst;
    }
    const auto hash = convert<QVariantHash>(value);
    if(!hash.isEmpty()) {
        QVariantList lst;
        lst.reserve(map.size());
        for(auto it = hash.begin(); it != hash.end(); ++it)
            lst.append(QVariant::fromValue(QVariantList({it.key(), it.value()})));
        return lst;
    }
    return QVariantList();
}

List::List(std::function<QVariant(const QVariant&)> concat, StreamBase* parent) :
    ProducerBase(parent), m_container(new Container<QVariantList>(this)) {
    QObject::connect(m_container, &StreamBase::next, this, &StreamBase::next);

    QObject::connect(m_container, &StreamBase::finished, this, [this](ProducerBase * origin) {
        if(origin != this) {
            emit finished(origin);
        }
    });
    QObject::connect(m_container, &StreamBase::waitOver, this, &StreamBase::waitOver);

    if(concat) {
        QObject::connect(m_parent, &StreamBase::next,  this, [this, concat](const QVariant & value) {
            m_pending = false;
            auto list = makeList(concat(value));
            if(!list.isEmpty()) {
                m_container->set(std::forward<QVariantList>(list));
            } else {
                m_container->append(value);
            }
            m_container->request(0);
        });
    } else {
        QObject::connect(m_parent, &StreamBase::next, this, [this, concat](const QVariant & value) {
            m_pending = false;
            auto list = makeList(value);
            if(!list.isEmpty()) {
                m_container->set(std::forward<QVariantList>(list));
            } else {
                m_container->append(value);
            }
            m_container->request(0);
        });

        if(m_parent) {
            QObject::connect(m_parent->producer(), &ProducerBase::completed, this,
            [this](ProducerBase * origin) {
                if(!isAlias(origin)) { //ensure its not me (or my subproviders) sending again
                    if(!m_container->wait()) {
                        emit completed(origin);
                    } else {
                        QObject::connect(m_container, &StreamBase::waitOver, this, [this, origin]() {
                            emit completed(origin);
                        },
                        Qt::QueuedConnection);
                    }
                }
            }, Qt::QueuedConnection);

            QObject::connect(m_parent, &StreamBase::finished, this, [this](ProducerBase * origin) {
                if(!m_container->wait()) {
                    emit finished(origin);
                } else {
                    QObject::connect(m_container, &StreamBase::waitOver, this, [this]() {
                        emit finished(this);
                    },
                    Qt::QueuedConnection);
                }
            }, Qt::QueuedConnection);
        }
    }
}


bool List::isAlias(const QObject* item) const {
    return item == this || item == m_container;
}

bool List::wait() const {
    return m_pending || m_container->hasData();
}

void List::cancel() {
    m_container->cancel();
    ProducerBase::cancel();
}

ProducerBase* List::producer() {
    return m_container;
}


template <typename T>
QSet<T > toSet(const QList<T>& lst) {
    QSet<T> o;
    for(const auto& i : lst) {
        o.insert(i);
    }
    return o;
}

Merge::Merge(const QList<StreamBase*>& sources, QObject* parent) : ProducerBase(parent), m_sources(toSet(sources)) {
    for(auto s : m_sources) {
        addChildren(s->producer());
        QObject::connect(s, &StreamBase::next, this, &ProducerBase::next);
        QObject::connect(s, &QObject::destroyed, this, [this](QObject * s) {
            m_sources.remove(static_cast<StreamBase*>(s)); //no metadata here
        });

        QObject::connect(s->producer(), &ProducerBase::completed, this, [this, s](ProducerBase * producer) {
            if(m_sources.contains(s)) {
                if(producer == s->producer()) {
                    //Here we look if there is nothing more emitted from the stream
                    //ie. its producer (and solely its) producer is complete
                    bool uniqProducer = true;
                    for(const auto& ss : m_sources) {
                        if(ss != s && producer == ss->producer()) {
                            uniqProducer = false;
                            break;
                        }
                    }
                    //...if so we remove it from the list
                    if(uniqProducer) {
                        m_sources.remove(s);
                    }
                }
            }
            if(m_sources.isEmpty()) {
                ProducerBase::complete();
            }
        }, Qt::QueuedConnection);
    }
}

bool Merge::wait() const {
    for(auto& source : m_sources) {
        if(source->wait()) {
            return true;
        }
    }
    return false;
}

ProducerBase* Merge::producer() {
    return this;
}

void Merge::complete() {
    for(auto& s : m_sources) {
        s->producer()->complete();
    }
}

void Merge::cancel() {
    for(auto& s : m_sources) {
        s->cancel();
    }
    ProducerBase::cancel();
}


Repeater::Repeater(RepeatFunction f, int ms, StreamBase* parent) : ProducerBase(parent) {init(f, ms);}
Repeater::Repeater(RepeatFunction f, int ms, QObject* parent) : ProducerBase(parent) {init(f, ms);}
Repeater::Repeater(RepeatFunction f, int ms, std::nullptr_t) : ProducerBase(nullptr) {init(f, ms);}

void Repeater::complete() {
    m_timer->stop();
    ProducerBase::complete();
}
void Repeater::request(int delay) {
    if(delay == RequestOne) {
        emit requestOne();
    } else {
        Q_ASSERT(delay >= 0);
        m_timer->start(delay);
        m_lastRequest = delay;
    }
}

bool Repeater::wait() const {
    return false;
}

void Repeater::defer() {
    m_timer->stop();
}

void Repeater::cancel() {
    m_timer->stop();
    ProducerBase::cancel();
}

void Repeater::init(RepeatFunction f, int ms) {
    m_timer = new QTimer(this);
    QObject::connect(m_timer, &QTimer::timeout, this, &Repeater::requestOne, Qt::QueuedConnection);
    m_timer->start(ms);
    QObject::connect(this, &ProducerBase::completed, [this](ProducerBase * origin) {
        if(isAlias(origin)) {
            m_timer->stop();
        }
    });
    QObject::connect(this, &Repeater::requestOne, [this, f]() {
        emit next(f());
    });
}


//Stream Producer

void Serializer :: init() {
    Q_ASSERT(!m_timer);
    m_timer = new QTimer(this);
    QObject::connect(this, &Serializer::requestOne, [this]() {
        onNext();
    });

    QObject::connect(m_timer, &QTimer::timeout, this, [this]() {
        if(hasData()) {
            emit Serializer::requestOne();
        } else {
            complete();
        }
    });
    QObject::connect(this, &Serializer::dataAdded, [this]() {
        if(m_delay != DoDefer && !m_timer->isActive()) {
            Q_ASSERT(m_delay >= 0);
            m_timer->start(m_delay);
        }
    });
    QTimer::singleShot(0, this, [this]() { //Calling virtual functions in c'tor is higly avoidable
        if(m_delay != DoDefer && !m_timer->isActive() && hasData()) { //we has to call even no hasData !
            request(m_delay);
        }
    });
}

bool Serializer::wait() const {
    return hasData();
}

Serializer::~Serializer() {
    m_timer->setParent(nullptr); //if timer is not in this thread, we cannot stop it here
    m_timer->deleteLater();     //but let it go to be destroyed in its own thread
}

void Serializer::cancel() {
    m_timer->stop();
    ProducerBase::cancel();
}

void Serializer::request(int delay) {
    if(hasData()) {
        if(delay < 0) { //Once or defer
            m_timer->stop();
            m_lastRequest = delay;
            emit requestOne();
        } else {
            m_delay = delay;
            m_timer->start(m_delay);
        }
    } else {
        complete();
    }
}

void Serializer::defer() {
    m_timer->stop();
    m_delay = DoDefer;
}


void Serializer::complete() {
    m_timer->stop();
    ProducerBase::complete();
}

void Serializer::request() {
    request(m_delay);
}

FuncProducer::FuncProducer(std::function<QVariant()> function, std::nullptr_t parent)  : Serializer(parent), m_f(function) {
}
FuncProducer::FuncProducer(std::function<QVariant()> function, QObject* parent)  : Serializer(parent), m_f(function) {
}
FuncProducer::FuncProducer(std::function<QVariant()> function, StreamBase* parent)  : Serializer(parent), m_f(function) {
}
bool FuncProducer::hasData() const {
    Q_ASSERT(!parent());
    return m_f && !m_called;
}
void FuncProducer::request(int ms) {
    m_called = false;
    Serializer::request(ms);
}
void FuncProducer::set(std::function<QVariant()> function) {
    m_called = function == nullptr;
    m_f = function;
    emit dataAdded();
}
void FuncProducer::cancel() {
    m_f = nullptr;
    ProducerBase::cancel();

}

void FuncProducer::onNext() {
    Q_ASSERT(!parent());
    if(!m_called) {
        m_called = true; //before! so set or request can have a side effect
        const auto v = m_f();
        if(v.isValid()) {
            emit next(v);
        } else {
            complete();
        }
    } else {
        complete();
    }
}


