#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include "axq.h"

using Data = QPair<QNetworkReply*, std::shared_ptr<QByteArray>>;
Q_DECLARE_METATYPE(Data);



std::shared_ptr<Axq::Stream> makeDataStream(QNetworkReply* reply,
                           const std::function<void (QByteArray&& str)>& onComplete,
                           const std::function<void (const QString& str)>& onError) {
    Data images = {reply, std::shared_ptr<QByteArray>(new QByteArray())};
    auto stream = std::shared_ptr<Axq::Stream>(new Axq::Stream);
    auto s = Axq::from(images)
            .waitComplete<Data>([](auto r) {
                        return Axq::from(r).wait(r.first, &QNetworkReply::readyRead)
                                .template onCompleted<Data>([](auto r) {
                            const auto bytes = r.first->readAll();
                            qDebug().nospace().noquote() << bytes.length();
                            *(r.second) += bytes;
                        });
                    })
            .waitComplete<Data>([stream](auto r) {
                        return Axq::from(r).wait(r.first, &QNetworkReply::finished)
                                .template onCompleted<Data>([stream](auto r) {
                                    qDebug() <<r.first->url() << r.second->length() << "done";
                                    stream->complete();
                                });
            })
            .waitComplete<Data>([stream](auto r) {
                        return Axq::from(r).wait(r.first, &QNetworkReply::errorOccurred)
                                .onCompleted([stream](auto thisStream) {
                                    thisStream.template meta<Data, Axq::Stream::This>([stream](const auto r, Axq::Stream s) {
                                        s.error( QString("Reply error %1").arg(r.first->errorString()).toLatin1().constData(), r.first->error(), true);
                                        r.first->deleteLater();
                                        stream->complete();
                                        return r;
                                });
                        });
                    })
            .waitComplete<Data>([](auto r){
                        return Axq::from(r).wait(r.first, &QNetworkReply::sslErrors);
                    })
            .onCompleted<Data>([onComplete](auto r) {
                    onComplete(std::move(*r.second));
                    r.first->deleteLater();
                    })
            .onError([onError](const Axq::Param& p, int code) {
                    onError(QString("When reading stream: %1 (%2)").arg(p.toString()).arg(code));
            });
    *stream = s;
    return stream;
}

int main(int argc, char *argv[]){
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;


    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    if(argc < 3) {
        qDebug() << "params: user_token project_token";
        return -1;
    }

    const QString userToken = argv[1];
    const QString projectToken = argv[2];

    QNetworkAccessManager* accessManager = new QNetworkAccessManager();
    QNetworkRequest request;
    request.setUrl(QUrl("https://api.figma.com/v1/files/" + projectToken + "/images"));
    request.setRawHeader("X-Figma-Token", userToken.toLatin1());
    auto reply = accessManager->get(request);

    const auto errf = [](const QString& err){
        qDebug() << err;
    };

    std::vector<std::shared_ptr<Axq::Stream>> streams;
    streams.push_back(makeDataStream(reply, [&](const QByteArray& bytes) {
        QJsonParseError perror;
        const auto json = QJsonDocument::fromJson(bytes, &perror);
        if(perror.error != QJsonParseError::NoError) {
            qDebug() << perror.errorString();
            return;
        }
        const auto images = json.object()["meta"].toObject()["images"].toObject();
        for(const auto& k : images.keys()) {
            QNetworkRequest request;
            request.setUrl(QUrl(images[k].toString()));
            request.setRawHeader("X-Figma-Token", userToken.toLatin1());
            auto reply = accessManager->get(request);
            streams.push_back(makeDataStream(reply, [k, &streams](const QByteArray bytes) {
                qDebug() << "get" << streams.size() << k << bytes.length();
            }, errf));
        }
    }, errf));


    return app.exec();
}
