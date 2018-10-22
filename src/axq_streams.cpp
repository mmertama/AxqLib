
#include "axq_streams.h"
#include "axq_producer.h"
//#include "axq_singleton.h"

#ifdef MEASURE_TIME
#include "axq.h"
#include <QElapsedTimer>
#include <QDebug>
QHash<QString, QElapsedTimer> global_elapsed;
void Axq::Measure::elapsed(const QString& name) {
    qDebug() << "Elapsed" << name <<  global_elapsed[name].restart();
}
#endif

#ifdef MEASURE_STREAMS
#include "axq.h"
#include <QDebug>
QList<QSet<QObject*>> global_streams;
void Axq::Measure::push() {
    global_streams.append(QSet<QObject*>());
}
void Axq::Measure::alloc(QObject* obj) {
    if(!global_streams.isEmpty()) {
        global_streams.last().insert(obj);
    }
}
void Axq::Measure::free(QObject* obj) {
    if(!global_streams.isEmpty()) {
        global_streams.last().remove(obj);
    }
}

static QString objectToString(QObject* o) {
    return o ? QString("%1(0x%2)").arg(o->metaObject()->className()).arg(reinterpret_cast<quint64>(o)) : "NULL";
}
static QString objectsToList(QObject* o, QSet<QObject*>& parentList) {
    if(o) {
        parentList.insert(o);
        return objectToString(o) + "->" +
               (parentList.contains(o->parent()) ? "*" : objectsToList(o->parent(), parentList));
    }
    return "NULL";
}
void Axq::Measure::pop() {
    if(!global_streams.isEmpty())
        QTimer::singleShot(0, []() {
        QSet<QObject*> parentList;
        for(const auto& obj : global_streams.last()) {
            qWarning().noquote() << "Leak:" << obj << "->" << objectsToList(obj->parent(), parentList) << " on " << global_streams.length();
        }
        global_streams.takeLast();
    });
}
#endif

#ifndef STREAM_ALLOC
#define STREAM_ALLOC(x)
#endif

#ifndef STREAM_FREE
#define STREAM_FREE(x)
#endif

using namespace Axq;

//Stream


StreamBase::StreamBase(StreamBase* parent) : QObject(parent), m_parent(parent) {
    STREAM_ALLOC(this)
    Q_ASSERT(parent);
    connectBaseHandlers();
    //re-send completed so it can reach its handler
    //do delayed as calling virtual functions in destructor is not encouraged
}

void StreamBase::connectFinished() {
    if(m_parent) {
        QObject::connect(m_parent, &StreamBase::finished, this, [this](ProducerBase * origin) {
            emit finished(origin);
        }, Qt::QueuedConnection);
    }
}


StreamBase::StreamBase(QObject* parent) : QObject(parent), m_parent(nullptr) {
    STREAM_ALLOC(this)
    connectBaseHandlers();
}

StreamBase::StreamBase(std::nullptr_t parent) : QObject(nullptr), m_parent(nullptr) {
    Q_UNUSED(parent);
    STREAM_ALLOC(this)
    connectBaseHandlers();
}

void StreamBase::delayedCall(std::function<void ()> f) {
    QTimer::singleShot(0, this, [f]() {
        f();
    });
}


void StreamBase::delayedCall(QObject* receiver, std::function<void ()> f) {
    QTimer::singleShot(0, receiver, [f]() {
        f();
    });
}

void StreamBase::connectBaseHandlers() {
    QObject::connect(this, &QObject::destroyed, this, &StreamBase::waitOver); //ensure parent not keep waiting to complete
    delayedCall([this]() {
        initConnections();
    });
}

void StreamBase::initConnections()  {
}

/*
 * Producers should not call setParent as that is assumed to be null if is having a StereamPrivate
*/
void StreamBase::addChildren(StreamBase* stream) {
    Q_ASSERT(stream != this);
    if(!stream->m_parent) {
        stream->m_parent = this;
    }
    m_children.insert(stream);
    QObject::connect(stream, &QObject::destroyed, this, [this, stream](QObject * o) {
        Q_ASSERT(stream == o); //but o may not have its metadata anymore
        removeChildren(stream);
    });
}

void StreamBase::removeChildren(StreamBase* stream) {
    Q_ASSERT(stream != this);
    m_children.remove(stream);
}

void StreamBase::childEvent(QChildEvent* event) {
    const bool isAdded = event->added();
    const auto ptr = event->child(); //may not have metadata!
    if(isAdded || event->removed()) {
        if(isAdded) {
            auto stream = static_cast<StreamBase*>(ptr);
            if(stream) {
                addChildren(stream);
            }
        } else {
            auto stream = static_cast<StreamBase*>(ptr); //only address matters as this may be destroyed
            if(stream) {
                removeChildren(stream);
            }
        }
    }
    QObject::childEvent(event);
}


StreamBase::~StreamBase() {
    STREAM_FREE(this)
}

ProducerBase* StreamBase::producer() {
    Q_ASSERT(m_parent);
    return m_parent->producer();
}

ProducerBase* StreamBase::root() {
    auto p = this;
    while(p->m_parent) {
        p = p->m_parent;
    }
    return qobject_cast<ProducerBase*>(p);
}

bool StreamBase::isAlias(const QObject* item) const {
    return item == this;
}

bool StreamBase::wait() const {
    return false;
}

void StreamBase::cancel() {
}

Error::~Error() {}

QVariant SimpleError::error() const  {return m_error;}
int SimpleError::errorCode() const   {return m_code;}
bool SimpleError::isFatal() const  {return m_fatal;}
