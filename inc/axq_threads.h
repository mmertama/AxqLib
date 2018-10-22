#ifndef CONSOLE_H
#define CONSOLE_H

#include <functional>
#include <memory>
#include <QThreadPool>
#include <QSet>
#include "axq_producer.h"
#include "axq_operators.h"


namespace Axq {

class AsyncProducer : public ProducerBase {
    Q_OBJECT
public:
    AsyncProducer(ProducerBase* hosted, std::shared_ptr<StreamBase> owner, QObject* parent = nullptr);
    ~AsyncProducer() Q_DECL_OVERRIDE;
    void request(int milliseconds) Q_DECL_OVERRIDE;
    void cancel() Q_DECL_OVERRIDE;
    void defer() Q_DECL_OVERRIDE;
    void complete() Q_DECL_OVERRIDE;
    bool wait() const Q_DECL_OVERRIDE;
signals:
    void requestSignal(int delayMs);
    void cancelSignal();
    void deferSignal();
    void completeSignal();
signals:
    void requestRead();
private:
    QThread m_thread;
    std::shared_ptr<StreamBase> m_owner;
};

class AsyncWatcher;

class AsyncOp : public ParentStream {
    Q_OBJECT
public:
    AsyncOp(AsyncWatcher* watcher);
    ProducerBase* producer() Q_DECL_OVERRIDE;
    void appendChild(StreamBase* child) Q_DECL_OVERRIDE;
signals:
    void ready();
    void finish();
private:
    AsyncWatcher* m_watcher = nullptr;
    QSet<StreamBase*> m_childCount;
};

class AsyncWatcher : public Operator {
    Q_OBJECT
public:
    AsyncWatcher(StreamBase* parent);
    AsyncOp* async() {return m_op.get();}
    bool wait() const Q_DECL_OVERRIDE;
private:
    std::unique_ptr<AsyncOp> m_op;
    QThread m_thread;
};

}

#endif
