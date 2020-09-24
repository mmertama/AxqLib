#include "figmaimageprovider.h"
#include <QImage>
#include <QImageReader>
#include <QBuffer>
#include <QFile>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include "axq.h"

class FigmaImages : public QQuickImageProvider {
public:
    FigmaImages(FigmaImageProvider* provider) : QQuickImageProvider(QQuickImageProvider::Image),
        m_provider(provider) {
    }

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override {
        Q_UNUSED(requestedSize);
        const auto index = id.toInt();
        const auto image = QImage::fromData(m_provider->m_images[index].first);
        *size = image.size();
        return image;
    }
    FigmaImageProvider* m_provider;
};

FigmaImageProvider::FigmaImageProvider(QObject* parent) : QObject(parent),
    m_provider(new FigmaImages(this)), m_access(new QNetworkAccessManager(this)) {
}

FigmaImageProvider::~FigmaImageProvider() {}

void FigmaImageProvider::set(QQmlEngine& engine) {
      engine.addImageProvider(m_name, m_provider);
}

QString FigmaImageProvider::name(int index) const {
    return "figma_" + m_images[index].second;
}

bool FigmaImageProvider::save(int index, const QString& dir) const {
    const auto fullName = dir + "/" + name(index);
    QFile out(fullName);
    if(out.open(QIODevice::WriteOnly)) {
        out.write(m_images[index].first);
        return true;
    }
    return false;
}

void FigmaImageProvider::addImage(const QByteArray& bytes, const QString& name) {
    QByteArray cp = bytes;
    QBuffer buffer(&cp);
    buffer.open(QIODevice::ReadOnly);
    const auto format = QImageReader::imageFormat(&buffer);
    if(format.length()) {
        m_images.append({bytes, name + '.' + format});
        emit countChanged();
    } else {
        qDebug() << "corrupted image" << name;
    }
}

int FigmaImageProvider::count() const {
    return m_images.count();
}

std::shared_ptr<Axq::Stream> makeDataStream(QNetworkReply* reply) {
    auto queue = new Axq::Queue();
    auto stream = std::shared_ptr<Axq::Stream>(new Axq::Stream(Axq::create(queue).buffer()));
    QObject::connect(reply, &QNetworkReply::readyRead, [reply, queue](){
        queue->push(reply->readAll());
    });
    QObject::connect(reply, &QNetworkReply::finished, [reply, queue]() {
        reply->deleteLater();
        queue->complete();
    });
    QObject::connect(reply, &QNetworkReply::errorOccurred, [reply, stream]() {
        stream->error( QString("Reply error %1").arg(reply->errorString()).toLatin1().constData(), reply->error(), true);
        reply->deleteLater();
        stream->complete();
    });
    QObject::connect(reply, &QNetworkReply::sslErrors, [reply, stream] {
        stream->error( QString("Reply error %1").arg(reply->errorString()).toLatin1().constData(), reply->error(), true);
        reply->deleteLater();
        stream->complete();
    });
    return stream;
}

void FigmaImageProvider::load() {
    QNetworkRequest request;
    request.setUrl(QUrl("https://api.figma.com/v1/files/" + m_projectToken + "/images"));
    request.setRawHeader("X-Figma-Token", m_userToken.toLatin1());
    auto reply = m_access->get(request);

    auto stream = makeDataStream(reply);
    stream->onBufferCompleted([this](const auto& bytes) {
        QJsonParseError perror;
        const auto json = QJsonDocument::fromJson(bytes, &perror);
        if(perror.error != QJsonParseError::NoError) {
            qDebug() << perror.errorString();
            return;
        }
        const auto images = json.object()["meta"].toObject()["images"].toObject();
        const auto iterator = Axq::from(images.keys()).template each<QString>([this, images](const auto& key){
            QNetworkRequest request;
            request.setUrl(QUrl(images[key].toString()));
            request.setRawHeader("X-Figma-Token", m_userToken.toLatin1());
            auto reply = m_access->get(request);
            auto imageStream = makeDataStream(reply);
            imageStream->onBufferCompleted([this, key](const auto& bytes){
                qDebug() << "add image" << bytes.length();
                addImage(bytes, key);
            });
            imageStream->onError([](const Axq::Param& p, int code) {
                qDebug() << QString("Error When reading image stream: %1 (%2)").arg(p.toString()).arg(code);
            });
        });
    });
    stream->onError([](const Axq::Param& p, int code) {
                    qDebug() << QString("Error When reading stream: %1 (%2)").arg(p.toString()).arg(code);
    });
}
