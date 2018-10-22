#include <QCoreApplication>
#include <QTextStream>
#include "axq.h"

#include <QDebug>

using namespace Axq;

int main(int argc, char *argv[]){
    QCoreApplication a(argc, argv);
    QTextStream qo(stdout);
    QTextStream qi(stdin);
    bool running = true;

    repeater<QString>(0, ([&qi, &running](){                                            //create a buzy repeater
        if(running){
            auto l = qi.readLine(); //readline is blocking!                             //that reads lines
            if(l == "quit")
                running = false;
            return l;
        }
        return QString();
    }))
                      .async()                                                          //as it is blocking, make it asynchronous
                      .onCompleted([&a](){
        a.exit(0);                                                                      //when done we exit aoo
    })
            .completeEach<QString>([&running](const QString&){
        return !running;                                                                //we are done when not running
    })
            .map(std::function<QStringList (const QString&)> ([](const QString& value)  //here is alternative way to tell compiler how to deduce types
                                                              ->QStringList{            //for each string we split to list of characters
       return value.split("", QString::SkipEmptyParts);
    }))
            .iterate()                                                                   //emit list items one by one
            .filter<QString>([](const QString& v){
        return v.length() > 0;                                                          //filter non-printable out
    })
            .map<QString, QString>([](const QString& v){         //do the actual rot13
        const auto c = v[0].toLower().unicode();
        return QChar((c >= 'a' && c <= 'z') ? ((c <= 'z' - 13) ? (c + 13) : 'a' + c + 12 - 'z') : c);
    })
            .buffer()                                                                   //collect values back to strings
            .each<QStringList>([&qo](const QStringList& out){
        qo <<  out.join("") << "\n";                                                     //...and print them
        qo.flush();
    });

     a.exec();
    return 0;
}
