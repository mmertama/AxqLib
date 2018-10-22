#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "axq.h"

int main(int argc, char *argv[]){
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    Axq::registerTypes(); //For QML

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
