#ifndef AXQ_SINGLETON_H
#define AXQ_SINGLETON_H

#include <QObject>
#include <QAtomicPointer>

namespace Axq {
class Singleton;
}

QBasicAtomicPointer ptr;

class Singleton : public QObject {
    Q_OBJECT
public:
    Singleton get() {
        m_this.testAndSetRelaxed(nullptr, new Singleton);
        return m_this.load();}
private:
    Singleton() : QObject(nullptr){
    }
private:
    static QAtomicPointer<Singleton> m_this;
};

Singleton::QAtomicPointer<Singleton> m_this = nullptr;

#endif // AXQ_SINGLETON_H
