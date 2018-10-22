#ifndef AXQ_PRIVATE_H
#define AXQ_PRIVATE_H

#include <memory>
#include "axq_streams.h"
#include "axq_producer.h"

namespace  Axq {
    class StreamPrivate : public QObject{
        Q_OBJECT
    public:
        StreamPrivate(const std::shared_ptr<StreamBase>& ptr);
        ~StreamPrivate();
    private slots:
        void completed();
    private:
        std::shared_ptr<StreamBase> m_ptr;
    };

    class Owner : public QObject {
        Q_OBJECT
    public:
        Owner(ProducerBase* p, std::function<void()> f) : QObject(p), m_onDelete(f){}
        ~Owner(){
            m_onDelete();
        }
    private:
        std::function<void()>  m_onDelete;
    };
}


#endif // AXQ_PRIVATE_H
