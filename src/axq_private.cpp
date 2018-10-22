#include "axq_private.h"

using namespace Axq;

StreamPrivate::StreamPrivate(const std::shared_ptr<StreamBase>& ptr) : QObject(nullptr), m_ptr(ptr) {
    /*
     * Memory management, does not let Stream's shared ptr to get null before completed
    */
    Q_ASSERT(!this->parent());
    QObject::connect(qobject_cast<ProducerBase*>(ptr.get()), &ProducerBase::completed, this, [this](ProducerBase * p) {
        if(p == m_ptr->producer()) {
            completed();
        }
    }, Qt::QueuedConnection);
}

void StreamPrivate::completed() {
    emit qobject_cast<ProducerBase*>(m_ptr.get())->finalized();
    this->deleteLater();
}


StreamPrivate::~StreamPrivate() {
}
