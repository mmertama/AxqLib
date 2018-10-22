#include <memory>
#include <QCoreApplication>
#include "unittest.h"

int main(int argc, char *argv[]){
    QCoreApplication a(argc, argv);
    std::unique_ptr<UnitTest> test(new UnitTest);
    QObject::connect(test.get(), &UnitTest::exit, &a, [&a](){
        a.quit();
    }, Qt::QueuedConnection);
    return a.exec();
}
