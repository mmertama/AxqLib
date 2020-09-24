#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "figmaimageprovider.h"


int main(int argc, char *argv[]){
    QGuiApplication app(argc, argv);

    std::unique_ptr<FigmaImageProvider> provider(new FigmaImageProvider());
    QQmlApplicationEngine engine;
    provider->set(engine);
    engine.rootContext()->setContextProperty("images", provider.get());

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    if(argc < 3) {
        qDebug() << "params: user_token project_token";
        return -1;
    }

    const QString userToken = argv[1];
    const QString projectToken = argv[2];

    provider->setProperty("userToken", userToken);
    provider->setProperty("projectToken", projectToken);
    provider->load();

    return app.exec();
}
