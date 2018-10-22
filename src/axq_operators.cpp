#include "axq_operators.h"
#include <QThread>

using namespace Axq;

void Operator::initConnections() {
    connectFinished();
}

Buffer::Buffer(int max, StreamBase* parent) :
    Operator(parent) {
    QObject::connect(m_parent, &StreamBase::next,  this, [this, max](const QVariant & value) {
        if(m_buffer.length() < max) {
            m_buffer.append(value);
        } else {
            emit next(m_buffer);
            m_buffer.clear();
        }
    });

    QObject::connect(m_parent->producer(), &ProducerBase::completed, this, [this](ProducerBase * origin) {
        if(origin == producer() && !m_buffer.isEmpty()) { //my producer
            emit next(m_buffer);
            m_buffer.clear();
        }
    });
}

void Buffer::cancel() {
    m_buffer.clear();
}


enum {Parent = 1, Child = 2};
Split::Split(StreamBase* parent) : ParentStream(parent), m_nexts({QVariant(), QVariant()}) {
    QObject::connect(m_parent, &StreamBase::next,  this, [this](const QVariant & v) {
        m_emits |= Parent;
        m_nexts[0] = v;
        tryNext();
    });
}

void Split::tryNext() {
    if(!m_child || ((m_emits & Parent)  && (m_emits & Child))) {
        emit next(m_nexts);
        m_emits = 0;
    }
}

StreamBase* Split::parent() {
    Q_ASSERT(m_parent);
    return m_parent;
}

void Split::appendChild(StreamBase* child) {
    Q_ASSERT(!m_child);
    m_child = child;
    QObject::connect(m_child, &StreamBase::next,  this, [this](const QVariant & v) {
        m_emits |= Child;
        m_nexts[1] = v;
        tryNext();
    });
}

void Split::cancel() {
    m_child->cancel();
}

Scan::Scan(std::function<QVariant()> getAcc, std::function<void (const QVariant& variant)> scan, StreamBase* parent) : Operator(parent) {
    QObject::connect(m_parent, &StreamBase::next,  this, [this, scan](const QVariant & value) {
        m_pending = true;
        scan(value);
    });

    QObject::connect(m_parent, &StreamBase::finished, this, [this, getAcc](ProducerBase * origin) {
        Q_UNUSED(origin);
        if(m_pending) {
            m_pending = false;
            emit next(getAcc());
            delayedCall([this]() {
                if(!m_pending) {
                    emit waitOver();
                }
            });
        }
    }, Qt::QueuedConnection); //as m_acc state set after next
}

bool Scan::wait() const {
    return m_pending;
}

void Scan::cancel() {
    m_pending = false;
}



Delay::Delay(StreamBase* parent, int ms)  : Operator(parent) {
    const auto delay = ms;
    QObject::connect(m_parent, &StreamBase::next,  this, [delay, this](const QVariant & value) {
        ++m_onWait;
        auto timer = get(delay);
        QObject::connect(timer, &QTimer::timeout, this, [this, value]() {
            emit next(value);
            --m_onWait;
            delayedCall([this]() {
                if(m_onWait == 0) {
                    emit waitOver();
                }
                if(m_finished && m_onWait == 0) {
                    emit finished(m_finished);
                }
            });
        });
    });
    QObject::connect(m_parent, &StreamBase::finished, this, [this](ProducerBase * origin) {
        m_finished = origin;
        if(m_onWait == 0) {
            emit finished(origin);
        }
    });
}

bool Delay::wait() const {
    return m_onWait > 0;
}

void Delay::cancel() {
    emit waitOver();
}

void Delay::initConnections()  {}

QTimer* Delay::get(int delay) {
    QTimer* timer = nullptr;
    for(const auto& t : m_timerPool) {
        if(!t->isActive()) {
            t->disconnect();
            timer = t;
        }
    }
    if(!timer) {
        timer = new QTimer(this);
        m_timerPool.append(timer);
    }
    timer->setSingleShot(true);
    Q_ASSERT(delay >= 0);
    timer->setInterval(delay);
    timer->start();
    QThread::msleep(1); //In some OSes(win): if delay operators are called within too small period
    //the timer resolution wont be able keep calls in order. Therefore we add
    //shortes possbile (due the same reason not a ms, but something more)
    //gap to keep the start timer moments distinct
    return timer;
}


Wait::Wait(StreamBase* parent) : Operator(parent) {
    QObject::connect(parent, &StreamBase::next, this, &StreamBase::next);
}

bool Wait::wait() const {
    return m_wait;
}
void Wait::unwait() {
    m_wait = false;
    emit waitOver();
}

