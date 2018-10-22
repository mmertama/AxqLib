#ifndef AXQ_QML_H
#define AXQ_QML_H

#include <QObject>
#include <QVariant>
#include <QJSEngine>
#include <QJSValue>

#include "axq.h"
#include "axq_streams.h"
#include "axq_operators.h"

class QQmlEngine;

namespace Axq {

class StreamBase;
class ProducerBase;
class EnvQML;
class Repeater;

class StreamQML : public QObject {
    Q_OBJECT
public:
    ~StreamQML();
    Q_INVOKABLE QVariant each(const QJSValue& caller);
    Q_INVOKABLE QVariant map(const QJSValue& caller);
    //async is missing
    //async (f) is missing
    Q_INVOKABLE QVariant split(const QJSValue& functionName, const QJSValue& p1 = QJSValue(), const QJSValue& p2 = QJSValue());
    Q_INVOKABLE QVariant filter(const QJSValue& caller);
    Q_INVOKABLE QVariant wait(const QJSValue& caller);
    Q_INVOKABLE QVariant completeEach(const QJSValue& caller);
    Q_INVOKABLE QVariant completeEach();
    Q_INVOKABLE QVariant scan(const QJSValue& begin, const QJSValue& caller);
    Q_INVOKABLE QVariant meta(int info, const QJSValue& caller);
    Q_INVOKABLE QVariant iterate(const QJSValue& caller = QJSValue());
    Q_INVOKABLE QVariant delay(int ms);
    Q_INVOKABLE QVariant buffer(const QJSValue& caller);
    Q_INVOKABLE QVariant buffer();
    Q_INVOKABLE QVariant onError(const QJSValue& caller);
    Q_INVOKABLE QVariant onComplete(const QJSValue& caller);
    Q_INVOKABLE virtual QVariant request(int delay = 0);
    Q_INVOKABLE QVariant defer();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void complete();
    Q_INVOKABLE void makeError(const QJSValue& error, int code = -999, bool isFatal = true);
    Q_INVOKABLE virtual void push(const QVariant& value = QVariant());
signals:
    void error(const QString& error);
public:
    StreamQML(EnvQML* env, StreamQML* parent);
    virtual StreamBase* stream() = 0;
    static StreamQML* findProducer(StreamQML* t, ProducerBase* p);
protected:
    EnvQML* m_env = nullptr;
    StreamQML* m_parent = nullptr;
};

class ProducerQML : public StreamQML {
    Q_OBJECT
public:
    //with value is missing
    Q_INVOKABLE QVariant onCompleted(const QJSValue& caller);
    Q_INVOKABLE QVariant request(int delay = 0) Q_DECL_OVERRIDE;
signals:
    void completed();
protected:
     ProducerQML(EnvQML* env, StreamQML* parent);
};

class RangeQML : public ProducerQML {
    Q_OBJECT
public:
    RangeQML(int begin, int end, int step, EnvQML* env, StreamQML* parent);
    StreamBase* stream() Q_DECL_OVERRIDE;
private:
    ProducerBase* m_range = nullptr;
};


class ContainerQML : public ProducerQML {
    Q_OBJECT
public:
    ContainerQML(const QVariant& item, EnvQML* env, StreamQML* parent);
    StreamBase* stream() Q_DECL_OVERRIDE;
private:
    ProducerBase* m_container = nullptr;
};

class QueueQML : public ProducerQML {
  Q_OBJECT
public:
    QueueQML(EnvQML* env, StreamQML* parent);
    Q_INVOKABLE void push(const QVariant& variant = QVariant()) Q_DECL_OVERRIDE;
    StreamBase* stream() Q_DECL_OVERRIDE;
private:
    QueueProducer* m_queue = nullptr;
};

class BufferQML : public StreamQML {
  Q_OBJECT
public:
    BufferQML(const QJSValue& caller, EnvQML* env, StreamQML* parent);
    StreamBase* stream() Q_DECL_OVERRIDE;
private:
    Buffer* m_buffer = nullptr;
    QJSValue m_onCaller;
};


class RepeaterQML : public ProducerQML {
    Q_OBJECT
public:
    RepeaterQML(const QJSValue& caller, int ms, EnvQML* env, StreamQML* parent);
    StreamBase* stream() Q_DECL_OVERRIDE;
private:
    QJSValue m_onCaller;
    Repeater* m_repeater;
    std::function<QVariant ()> makeCall(const QJSValue& caller, RepeaterQML* r);
};


class MergeQML : public ProducerQML {
    Q_OBJECT
public:
    MergeQML(const QJSValueList& sources, EnvQML* env, StreamQML* parent);
    StreamBase* stream() Q_DECL_OVERRIDE;
private:
    Merge* m_merge;
};

class IteratorQML : public ProducerQML {
    Q_OBJECT
public:
    IteratorQML(const QJSValue& objects, EnvQML* env, StreamQML* parent);
    StreamBase* stream() Q_DECL_OVERRIDE;
private:
    Iterator<QObject*>* m_iterator;
};


class NoneQML : public StreamQML{
    Q_OBJECT
public:
    NoneQML(EnvQML* env, StreamQML* parent);
protected:
    StreamBase* stream() Q_DECL_OVERRIDE;
private:
    StreamBase* m_nothing = nullptr;

};

class ScanQML : public StreamQML {
    Q_OBJECT
public:
    ScanQML(const QJSValue& begin, const QJSValue& caller, EnvQML* env, StreamQML* parent);
protected:
    StreamBase* stream() Q_DECL_OVERRIDE;
private:
    QJSValue m_acc;
    QJSValue m_caller;
    StreamBase* m_scan = nullptr;
};

class SplitQML : public StreamQML {
    Q_OBJECT
public:
    SplitQML(const QJSValue& functionName, const QJSValue& p1, const QJSValue& p2, EnvQML* env, StreamQML* parent);
protected:
    StreamBase* stream() Q_DECL_OVERRIDE;
private:
    Split* m_split = nullptr;
};


class WaitQML : public StreamQML {
    Q_OBJECT
public:
    WaitQML(const QJSValue& caller, EnvQML* env, StreamQML* parent);
protected:
    StreamBase* stream() Q_DECL_OVERRIDE;
private:
    QJSValue m_caller;
    StreamBase* m_wait = nullptr;
};

class DelayQML : public StreamQML {
    Q_OBJECT
public:
    DelayQML(int ms, EnvQML* env, StreamQML* parent);
protected:
    StreamBase* stream() Q_DECL_OVERRIDE;
private:
    StreamBase* m_delay = nullptr;
};

class EnvQML : public QObject{
    Q_OBJECT
public:
    enum Info {Index = Information::Index};
    Q_ENUM(Info)
public:
    EnvQML(QQmlEngine* qml, QJSEngine* js, QObject* parent = nullptr);
    Q_INVOKABLE QVariant range(const QVariant& begin = QVariant(0), const QVariant& end = QVariant(), const QVariant& step = QVariant());
    Q_INVOKABLE QVariant from(const QVariant& item);
    Q_INVOKABLE QVariant queue();
    Q_INVOKABLE QVariant repeater(const QJSValue& caller, int ms);
    Q_INVOKABLE QVariant iterator(const QJSValue& object);
    Q_INVOKABLE QVariant merge(const QJSValueList& sources);
    QQmlEngine& qml() {return *m_qml;}
    QJSEngine& js() {return *m_js;}
signals:
    void error(const QString& errorString);
private:
    QVariant makeError(const QString& errorString);
private:
    QQmlEngine * m_qml;
    QJSEngine * m_js;
};


}


#endif // AXQ_QML_H
