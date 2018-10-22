#include "inc/axq_threads.h"
#include <QFutureWatcher>
#include <QtConcurrent>

using namespace Axq;

AsyncProducer::AsyncProducer(ProducerBase* hosted, std::shared_ptr<StreamBase> owner, QObject* parent) : ProducerBase(parent),
    m_owner(owner) {

    hosted->moveToThread(&m_thread);

    QObject::connect(hosted, &ProducerBase::completed, this, [this, hosted](ProducerBase * origin) {
        if(hosted != origin) {
            complete();
        }
    });

    QObject::connect(hosted, &StreamBase::waitOver, this, &StreamBase::waitOver);

    QObject::connect(hosted, &StreamBase::next, this, [this](const QVariant & v) {
        emit StreamBase::next(v);
    });

    QObject::connect(this, &AsyncProducer::completeSignal, hosted, [hosted]() {
        hosted->complete();
    });


    QObject::connect(this, &AsyncProducer::requestSignal, hosted, [hosted, this](int ms) {
        m_lastRequest = ms;
        hosted->request(ms);
    });

    QObject::connect(this, &AsyncProducer::deferSignal, hosted, [hosted]() {
        hosted->defer();
    });

    QObject::connect(this, &AsyncProducer::cancelSignal, hosted, [hosted]() {
        hosted->cancel();
    });


    m_thread.start();
}

bool AsyncProducer::wait() const {
    return m_thread.isRunning();
}

AsyncProducer::~AsyncProducer() {
    m_thread.quit();
    m_thread.wait();
}

void AsyncProducer::request(int delayMs) {
    emit requestSignal(delayMs);
}

void AsyncProducer::defer() {
    emit deferSignal();
}

void AsyncProducer::cancel() {
    emit cancelSignal();
}

void AsyncProducer::complete() {
    ProducerBase::complete();
    emit completeSignal();
}

#define TO_STR(x) (#x)


void AsyncOp::appendChild(StreamBase* child) {
    m_childCount.insert(child);
    QObject::connect(child, &StreamBase::next, this, [this](const QVariant & v) {
        emit m_watcher->StreamBase::next(v);
    });
    QObject::connect(child, &QObject::destroyed, this, [this](QObject * obj) {
        m_childCount.remove(static_cast<StreamBase*>(obj));
        if(m_childCount.isEmpty()) {
            emit finish();
        }
    });
    QObject::connect(this, &AsyncOp::finished, this, [this](ProducerBase * origin) {
        if(m_childCount.isEmpty()) {
            emit finish();
            return;
        }
        for(auto child : m_childCount) {
            if(child->wait()) {
                QObject::connect(child, &StreamBase::waitOver, this, [this, origin]() {
                    emit finished(origin);
                }, Qt::QueuedConnection);
                return;
            } else {

                m_childCount.remove(child);
                if(m_childCount.isEmpty()) {
                    emit finish();
                    return;
                }
            }
        }
    }, Qt::QueuedConnection);
    emit ready();
}

AsyncOp::AsyncOp(AsyncWatcher* watcher) : ParentStream(nullptr), m_watcher(watcher) {
}

ProducerBase* AsyncOp::producer() {
    Q_ASSERT(m_watcher);
    return m_watcher->producer();
}



AsyncWatcher::AsyncWatcher(StreamBase* parent) : Operator(parent), m_op(new AsyncOp(this)) {
    auto ao = m_op.get(); //qobject_cast not working

    QObject::connect(m_parent, &StreamBase::next, ao, [ao](const QVariant & v) {
        ao->AsyncOp::next(v);
    });


    QObject::connect(ao, &AsyncOp::ready, this, [this, ao]() {
        ao->moveToThread(&m_thread);
        m_thread.start();
    });

    QObject::connect(ao, &AsyncOp::finish, &m_thread, &QThread::quit);
    QObject::connect(&m_thread, &QThread::finished, this, &StreamBase::waitOver);
    QObject::connect(this, &AsyncWatcher::finished, ao, &AsyncOp::finished);
    QObject::connect(m_parent, &StreamBase::finished, this, [this](ProducerBase * origin) {
        Q_UNUSED(origin);
        if(!m_thread.isRunning()) { //i.e. asyncOp::appendChild has to be called
            QEventLoop waitLoop(this);
            QObject::connect(&m_thread, &QThread::started, &waitLoop, &QEventLoop::quit);
        }
        emit finished(origin);
    });
}


bool AsyncWatcher::wait() const {
    return m_thread.isRunning(); // && m_waitNext;
}



