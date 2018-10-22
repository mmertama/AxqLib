#include <random>
#include <algorithm>
#include <vector>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QTextStream>
#include <QTimer>
#include <QFile>
#include <QThread>
#include <QMetaMethod>
#include <QTime>
#include <QVector>
#include "unittest.h"
#include "axq.h"

#include <QDebug>

//Ignore there for testing: Axq::registerTypes(), is for QML only .push() and is referered via ->

//Due simplicity of parser something has to be renamed
#define _renamed_split split

template <typename T>
void _print(QTextStream& stream, T p0) {
    stream << p0;
}

template <typename T, typename ...Args>
void _print(QTextStream& stream, T p0, Args... args) {
    stream << p0;
    _print(stream, args...);
}



template <typename ...Args>
void print(Args... args) {
    QTextStream qo(stdout);
    _print(qo, args...);
    qo.flush();
}

template <typename iterator>
void print(iterator begin, iterator end, const QString& sep = ",") {
    if(begin != end) {
        auto it = begin;
        auto last = end - 1;
        for(; it != last; ++it) {
            print(*it);
            print(sep);
        }
        if(it != end) {
            print(*it);
        }
    }

}

Q_DECLARE_METATYPE(std::vector<bool>*)
Q_DECLARE_METATYPE(QList<int>)
Q_DECLARE_METATYPE(QRegularExpressionMatchIterator)
Q_DECLARE_METATYPE(QRegularExpressionMatch)

static int countMethods(const char* name, const QMetaObject& meta) {
    int count = 0;
    const auto methodCount = meta.methodCount();
    for(int i = 0; i < methodCount; i++) {
        auto method = meta.method(i);
        if(method.isValid() && method.name().startsWith(name)) {
            ++count;
        }
    }
    return count;
}

template <typename T, typename It>
T reduce(It begin, It end, const T& v0, std::function<T(const T& a, decltype(*begin))> f) {
    if(begin == end) {
        return v0;
    }
    T acc = f(v0, *begin);
    ++begin;
    for(; begin != end; ++begin) {
        acc = f(acc, *begin);
    }
    return acc;
}

static bool numsum(const QString& v1, const QString& v2) {
    const auto l1 = v1._renamed_split(',');
    const auto l2 = v2._renamed_split(',');
    int c1 = reduce<int>(l1.begin(), l1.end(), 0, [](int acc, const QString & s) {return acc + s.toInt();});
    int c2 = reduce<int>(l2.begin(), l2.end(), 0, [](int acc, const QString & s) {return acc + s.toInt();});
    return c1 == c2;
}


void UnitTest::expectTest(const QString& expectation) {
    m_buffer.clear();
    m_buffer.append(expectation);
}


void UnitTest::verifyTest(std::function<bool (const QString&, const QString&)> match) {
    if(m_buffer.isEmpty()) {
        print("\n");
        const auto method = metaObject()->method(m_methodIndex);
        qt_assert("Invalid test: No expectation set",
                  method.name(), m_currentLine + 1);
    }
    const auto test = m_buffer.takeFirst();
    const auto output = m_buffer.join(QString(""));
    const auto isMatch = match ? match(test, output) : test.simplified() == output.simplified();
    if(!isMatch) {
        print("\n");
        const auto method = metaObject()->method(m_currentMethod);
        qt_assert((QString("\nExpected\n%1\ndoes not match with\n%2\n").arg(test).arg(output)).toStdString().c_str(),
                  method.name(), m_currentLine + 1);
    }
    m_currentLine = -1;
    print("\nverified\n");
}

constexpr char name[] = "test";

int lineNo(const QString& function) {
    const auto paths = QString(SOURCEPATH)._renamed_split(";");
    const QRegularExpression re(QString("\\s+%1\\s*\\(").arg(function));
    for(const auto& p : paths) {
        QFile so(p);
        so.open(QIODevice::ReadOnly);
        int i = 0;
        while(!so.atEnd()) {
            if(re.match(so.readLine()).hasMatch()) {
                return i;
            }
            ++i;
        }
    }
    return -1;
}

static QStringList parseArgument(const QChar shortName, const QString& longName, int after, int len) {
    const auto args = QCoreApplication::arguments();
    const QRegularExpression shortRe(R"(^-(\S)$)");
    const QRegularExpression longRe(R"(^--(\S+)$)");
    const auto paramAt = [after, len](int index, const QStringList & args) {
        return args.mid(index + after, len);
    };
    for(int i = 0; i < args.length(); i++) {
        const QString arg(args[i]);
        const auto sm = shortRe.match(arg);
        if(sm.hasMatch() && sm.captured(1)[0] == shortName) {
            return paramAt(i, args);
        }
        const auto lm = longRe.match(arg);
        if(lm.hasMatch() && lm.captured(1) == longName) {
            return paramAt(i, args);
        }
    }
    return QStringList();
}


UnitTest::UnitTest() : m_testCount(countMethods(name, *metaObject())) {
    QVector<int> tests;
    const auto ts = parseArgument('t', "test", 1, -1);

    std::transform(ts.begin(), ts.end(), std::back_inserter(tests), [](const QString & t) {return t.toInt();});

    QObject::connect(this, &UnitTest::next, this, [this, tests]() {
        const auto meta = metaObject();
        const auto methodCount = meta->methodCount();

        Q_ASSERT_X(m_currentLine < 0 || !m_buffer.isEmpty(),  meta->method(m_currentMethod).name(), "is not verified");

        if(m_methodIndex < methodCount) {
            auto method = meta->method(m_methodIndex);
            if(method.isValid() && method.name().startsWith(name)) {
                ++m_currentTest;
                if(tests.isEmpty() || tests.contains(m_currentTest)) {
                    m_buffer.clear();
                    print(QString("\nCalling: %1 of %2 %3 \n").arg(m_currentTest).arg(m_testCount).arg(QString(method.name())));
                    m_currentLine = lineNo("UnitTest::" + QString(method.name()));
                    m_currentMethod = m_methodIndex;
                    method.invoke(const_cast<UnitTest*>(this), Qt::QueuedConnection);
                } else { emit next(); }
            } else {
                emit next();
            }
        } else {
            print("Tests passed, exiting\n");
            emit exit();
            return;
        }
        ++m_methodIndex;
    }, Qt::QueuedConnection);

    QObject::connect(this, &UnitTest::doxRead, [this]() {
        print("\n");
        emit next();
    });

    if(tests.isEmpty() || tests.contains(6)) { // a bit ugly, sorry
        m_dox = new TestData(this);
        readDox();
    } else {
        emit next();
    }
}

void UnitTest::test_checkApi() {
    auto paths = QString(SOURCEPATH)._renamed_split(";");
    auto found = new QStringList(m_dox->briefs());
    expectTest("");
    Axq::from(paths)
    .own(found)
    .waitComplete<QString>([found, this](const QString & name) {
        auto open = [name]() {
            auto f = new QFile(name);
            Q_ASSERT(f->open(QIODevice::ReadOnly));
            return f;
        };
        return Axq::read(open(), Axq::Stream::Slurp)
        .map<QRegularExpressionMatchIterator, QString>([](const QString & fileContent) {
            return QRegularExpression(R"((Axq::(?<static>[a-zA-Z_][a-zA-Z0-9_]*))|\.((?<call>[a-zA-Z_][a-zA-Z0-9_]*)\s*(\(|<)))").globalMatch(fileContent);
        })
        .filter<QRegularExpressionMatchIterator>([](const QRegularExpressionMatchIterator & it) {
            return it.hasNext();
        })
        .waitComplete<QRegularExpressionMatchIterator>([found, this](const QRegularExpressionMatchIterator & it) {
            return Axq::iterator<QRegularExpressionMatch, QRegularExpressionMatchIterator>(it)
            .filter<QRegularExpressionMatch>([](const QRegularExpressionMatch & match) {
                return match.hasMatch();
            })
            .map<QString, QRegularExpressionMatch>([](const QRegularExpressionMatch & match) {
                return !match.captured("static").isNull() ? match.captured("static") : match.captured("call");
            })
            .each<QString>([found, this](const QString & name) {
                if(m_dox->hasBrief(name)) {
                    found->removeOne(name);
                }
            });
        });
    }).onCompleted([found, this]() {
        print("Missing tests: ", found->join(','), "\n");
        appendTest(found->join(','));
        verifyTest();
        emit next();
    });
}

void UnitTest::test_defer() {
    expectTest("Wait...doing...done...dummy deleted");
    auto dummy = new Dummy([this]() {
        print("dummy deleted\n");
        appendTest("...dummy deleted");
        verifyTest();
        emit next();
    }
                          );
    auto stream = Axq::from(dummy).defer().onCompleted([this]() {
        print("done\n");
        appendTest("done");
    });
    print("Wait...");
    appendTest("Wait...");
    QTimer::singleShot(1000, [stream, this]() mutable {
        print("doing...");
        appendTest("doing...");
        stream.request(1000);
    });
}

#define STRFY(x) #x

void UnitTest::readDox() {
    auto paths = QString(INCLUDEPATH)._renamed_split(";");
    paths.append("");
    Axq::from(paths)
    .meta<QString, Axq::Stream::This>([](const QString & header, Axq::Stream s) {
        if(header.isEmpty()) {
            s.error("header not found");
        }
        return header;
    })
    .map<QString, QString>([](const QString & path) {
        return path + "/" + "axq.h";
    })
    .filter<QString>([](const QString & header) {
        return QFile::exists(header);
    })
    /*
     * Calling cancel is one way to stop, complete([](value){return true;}) is maybe more clean
    */
    .meta<QString, Axq::Stream::This>([this](const QString & header, Axq::Stream s) {
        auto f = new QFile(header);
        if(!f->open(QIODevice::ReadOnly)) {
            delete f;
            s.error("Cannot open");
        } else {
            enum class State {Out, In, Brief, Param, Return};
            auto inDoxComment = new State(State::Out);
            auto current = new QString;
            Axq::read(f)
            .own(current)
            .own(inDoxComment)
            .each<QString>([inDoxComment, this, current](const QString & line) {
                if(*inDoxComment != State::Out) {
                    const QRegularExpression blockCommentEnd(R"(\*/)");
                    const QRegularExpression brief(R"(\*\s*@function\s*(\w*)(.*))");
                    const QRegularExpression param(R"(\*\s*@param\s*(.*))");
                    const QRegularExpression returnValue(R"(\*\s*@return\s*(.*))");
                    if(blockCommentEnd.match(line).hasMatch()) {
                        *inDoxComment = State::Out;
                    } else {
                        const auto bm = brief.match(line);
                        if(bm.hasMatch()) {
                            *current = bm.captured(1);
                            m_dox->setBrief(*current, bm.captured(2));
                            *inDoxComment = State::Brief;
                        } else if(param.match(line).hasMatch()) {
                            m_dox->setData(*current, "param", bm.captured(1));
                            *inDoxComment = State::Param;
                        } else if(returnValue.match(line).hasMatch()) {
                            *inDoxComment = State::Return;
                            m_dox->setData(*current, "return", bm.captured(1));
                        } else if(*inDoxComment == State::Brief) {
                            m_dox->setData(*current, "brief", line);
                        } else if(*inDoxComment == State::Param) {
                            m_dox->setData(*current, "param", line);
                        } else if(*inDoxComment == State::Return) {
                            m_dox->setData(*current, "return", line);
                        }
                    }
                } else {
                    const QRegularExpression doxCommentStart(R"(/\*\*)");
                    if(doxCommentStart.match(line).hasMatch()) {
                        *inDoxComment = State::In;
                    }
                }
            })
            .onCompleted([this]() {
                emit doxRead();
            });
            s.defer();
        }
        return header;
    })
    .onError<QString>([this](const QString & err, int code) {
        print(err, code);
        print("\n");
        emit next();
    });
}

void UnitTest::test_async() {
    expectTest("cat is THREAD_ONE, but THREAD_TWO, and then back THREAD_ONE");
    auto threadHandle = []() {
        return QString::number(reinterpret_cast<quint64>(QThread::currentThreadId()), 16);
    };

    Axq::create<QString>([]()->QString{
        return "cat";
    })
    .map<QString, QString>([threadHandle](const QString & value) {
        return value + " is " + threadHandle();
    })
    .async(&Axq::Stream::map<QString, QString>, [threadHandle](const QString & value) {
        return value + ", but " +  threadHandle();
    }).each<QString>([threadHandle, this](const QString & value) {
        print(value + ", and then back " +  threadHandle() + "\n");
        appendTest(value + ", and then back " +  threadHandle());
    }).onCompleted([this]() {
        print("\n");
        verifyTest([](const QString&, const QString & output) {
            const QRegularExpression re1(R"(cat\s+is\s+([a-z0-9]+),\s+but\s+([a-z0-9]+),\s+and\s+then\s+back\s+(\1))");
            const QRegularExpression re2(R"(cat\s+is\s+([a-z0-9]+),\s+but\s+([a-z0-9]+),\s+and\s+then\s+back\s+(\2))");
            return re1.match(output).hasMatch() && !re2.match(output).hasMatch();
        });
        emit next();
    });
}


void UnitTest::test_merge() {
    STREAM_START_MEM
    expectTest("1 2 3 1 2 1 3 1 2 4 1 3 2 5 1 2 1 3 1 2 3 1 4 2 1 3 1 2 5 1 3 2 1 2 1 4 3 1 2 1 3 2 1 5 3 1 2 1 4 2 3 1 ");
    auto r0 = Axq::repeater(121, 1);
    auto r1 = Axq::repeater(170, 2);
    auto r2 = Axq::repeater(220, 3);
    auto r3 = Axq::repeater(590, 4);
    auto r4 = Axq::repeater(740, 5);
    Axq::merge({r0, r1, r2, r3, r4}).each<int>([this](int p) {
        print(p, " ");
        appendTest(p, " ");
    })
    .meta<int, Axq::Stream::Index>([](int, int count) {
        return  count;
    })
    .completeEach<int>([](int count) {
        return count > 50;
    })
    .onCompleted([this]() {
        print("\n");
        verifyTest(numsum);
        emit next();
        STREAM_CHECK_MEM
    });
}



/*Sleep Sort!*/

void UnitTest::test_sleepSort() {
    std::srand(100);
    std::vector<int> ints(25);
    std::generate(ints.begin(), ints.end(), []() {return std::rand() % 100;});
    std::vector<int> cp = ints;
    std::sort(cp.begin(), cp.end());
    expectTest(cp.begin(), cp.end());
    print("\n");

    QList<Axq::Stream> streams;
    for(const auto& v : ints) {
        streams.append(Axq::from(v).delay(v * 100));    //has to be big enough - otherwise system delays would affect to the sleeps
    }

    auto out = new std::vector<int>;
    Axq::merge(streams)
    .own(out)
    .each<int>([out](int i) {
        print(".");
        out->push_back(i);
    })
    .onCompleted([this, out]() {
        print("\n");
        print(out->begin(), out->end());
        appendTest(out->begin(), out->end());
        verifyTest(numsum);
        print("\n");
        emit next();
    });
}

/*
 * Sleep sort,
*/
void UnitTest::test_sleepSort2() {
    std::vector<int> ints(25);
    std::generate(ints.begin(), ints.end(), []() {return std::rand() % 100;});
    expectTest("ok");

    QList<Axq::Stream> streams;
    for(const auto& v : ints) {
        streams.append(Axq::from(v).delay(v * 200));
    }

    Axq::merge(streams)
    .meta<int, Axq::Stream::Index>([this](int value, int count) {
        if(count >= 1) {
            print(",");
        } else { appendTest("ok"); }
        return value;
    })
    .each<int>([](int value) {
        print(value);
    })
    .onCompleted([this]() {
        verifyTest();
        print("\n");
        emit next();
    });
}

void UnitTest::test_scan() {
    STREAM_START_MEM
    STREAM_ELAPSED("test_scan")
    expectTest("\"shadow\" exists 3 times");
    Axq::from(QString(R"(I have a little shadow that goes in and out with me,
              And what can be the use of him is more than I can see.
              He is very, very like me from the heels up to the head;
              And I see him jump before me, when I jump into my bed.

              The funniest thing about him is the way he likes to grow—
              Not at all like proper children, which is always very slow;
              For he sometimes shoots up taller like an india-rubber ball,
              And he sometimes goes so little that there’s none of him at all.

              He hasn’t got a notion of how children ought to play,
              And can only make a fool of me in every sort of way.
              He stays so close behind me, he’s a coward you can see;
              I’d think shame to stick to nursie as that shadow sticks to me!

              One morning, very early, before the sun was up,
              I rose and found the shining dew on every buttercup;
              But my lazy little shadow, like an arrant sleepy-head,
              Had stayed at home behind me and was fast asleep in bed.)"))
    .iterate<QString, QString>([](const QString & s) {
        return s._renamed_split(" ");
    })
    .map<QString, QString>([](const QString & word) {
        auto str = word.toLower().replace(QRegularExpression(R"([^a-zA-Z])"), "");
        return str;
    })
    .filter<QString>([](const QString & word) {
        return !word.isEmpty();
    })
    .scan<QHash<QString, int>, QString>(QHash<QString, int>(), [](QHash<QString, int>& acc, const QString & word) {
        acc[word]++;
    })
    .iterate() //converts QHash to key-value pairs in ParamList
    .buffer()   //collects those pairs into ParamList
    .map<Axq::ParamList, Axq::ParamList>([](Axq::ParamList lst) {
        std::sort(lst.begin(), lst.end(), [](const Axq::Param & a, const Axq::Param & b) {
            const auto pairA = a.value<Axq::ParamList>();
            const auto pairB = b.value<Axq::ParamList>();
            QString key; int countA, countB;
            Axq::tie(pairA, key, countA);
            Axq::tie(pairB, key, countB);
            return countA > countB;
        });
        return lst;
    })
    .iterate()
    .each<Axq::ParamList>([this](const Axq::ParamList & pair) {
        QString key; int count;
        Axq::tie(pair, key, count);
        print("\"", key, "\" exists ", count, " times\n");
        if(key == "shadow") {
            appendTest("\"", key, "\" exists ", count, " times\n");
        }
    }).onCompleted([this]() {
        print("\n");
        verifyTest();
        emit next();
        STREAM_ELAPSED("test_scan")
        STREAM_CHECK_MEM
    });
}

void UnitTest::test_erastothenes() {
    expectTest("99787 99793 99809 99817 99823 99829 99833 99839 99859 99871 99877 99881 99901 99907 99923 99929 99961 99971 99989 99991 ");
    using Integer = unsigned long long;
    constexpr ulong len = 100000;
    auto bitset = new std::vector<bool>({true, false, false, false}); //1-4
    bitset->resize(len, false);
    Axq::range(2, static_cast<int>(std::ceil(std::sqrt(len))))
    .own(bitset)
    .waitComplete<ulong>([bitset, len](ulong v) { // some compilers expect len to be captured so it can be accessd inner
        return  Axq::range(v + v, len, v)
        .each<ulong>([bitset](ulong v) {
            (*bitset)[v] = true;
        });
    }).onCompleted([this, bitset, len](const Axq::Stream & stream) {
        //for(int i = 0; i < std::distance(bitset->begin(), bitset->end()) ; i++)
        // qDebug() << (!(*bitset)[i] ? QString::number(i) : " ");
        Axq::iterator<bool>(bitset->begin(), bitset->end())
        .take(stream)
        .meta<Axq::ParamList, bool, Axq::Stream::Index>([](bool b, ulong index) {
            /* static int t = 0;
             static int f = 0;
             if(b) t++; else f++;
             qDebug() << "b" << b << index ; */

            return Axq::params(b, index);
        })
        .filter<Axq::ParamList>([len](const Axq::ParamList & param) {
            const auto b = Axq::get<0, bool>(param);
            //  qDebug() << "b" << b << Axq::get<0, int>(param);
            return !b;
        })
        .scan<QList<Integer>, Axq::ParamList>(QList<Integer>(), [](QList<Integer>& acc, const Axq::ParamList & param) {
            const auto count = Axq::get<1, Integer>(param);
            //   qDebug() << "p" << count;
            acc.append(count);
        }).completeEach<QList<Integer>>([this](const QList<Integer>& primes) {
            auto primesPtr = new QList<Integer>(primes);
            Axq::iterator<Integer>(std::max(primesPtr->begin(), primesPtr->end() - 20), primesPtr->end())
            .each<Integer>([this](Integer prime) {
                print(prime, " ");
                appendTest(prime, " ");
            })
            .onCompleted([this]() {
                print("\n");
                verifyTest();
                emit next();
            });
            return true;
        });
    });
}

void UnitTest::test_split() {
    expectTest("Quack kcauQQuack kcauQQuack kcauQQuack kcauQQuack kcauQQuack kcauQQuack kcauQQuack kcauQQuack kcauQQuack kcauQQuack kcauQQuack kcauQ");
    auto queue = new Axq::Queue();
    auto timer = new QTimer(queue);
    timer->start(1000);
    QObject::connect(timer, &QTimer::timeout, queue, [queue]() {
        queue->push<QString>("Quack");
    });
    Axq::create(queue)
    .split(&Axq::Stream::map<QString, QString>, [](QString str) {
        std::reverse(str.begin(), str.end());
        return str;
    })
    .map<QString, Axq::ParamList>([](const Axq::ParamList & params) {
        return Axq::get<0, QString>(params) + " " + Axq::get<1, QString>(params);
    })
    .each<QString>([this](const QString & s) {
        print(s);
        appendTest(s);
    })
    .meta<bool, QString, Axq::Stream::Index>([](const QString&, int count) {
        return count > 10;
    })
    .completeEach<bool>([](bool t) {
        return t;
    })
    .onCompleted([this]() {
        verifyTest();
        print("\n");
        next();
    });
}

void UnitTest::test_error() {
    QTime start;
    start.start();
    expectTest("ooopsooopsooopsooopsooopsconfused");
    auto repeater = Axq::repeater<QTime>(643, []() { //this <QTime> is important, otherwise the function is considered to be repeated, not its output
        return QTime::currentTime();
    })
    .meta<QTime, Axq::Stream::This>([start](const QTime & t, Axq::Stream s) {
        if(start.elapsed() >= 2000) {
            s.error("ooops");
        }
        if(start.elapsed() > 5000) {
            s.error("confused", 999, true);
        }
        return t;
    })
    .onError<QString>([this](const QString & err, int code) {
        print(err, " code:", code);
        appendTest(err);
        if(code == 999) {
            print("\n");
            verifyTest();
            emit next();
        }
    })
    .onCompleted([]() {
        Q_ASSERT(false);
    });
}

void UnitTest::test_flush() {
    STREAM_START_MEM;
    expectTest("ping-1 ping-2 ping-3 ping-4 ping-5 ping-6 ping-7 ping-8 ping-9 ping-10 pong-11");
    auto p = new Axq::Queue();
    auto t = new QTimer();
    t->start(100);
    static int count = 1;
    QObject::connect(t, &QTimer::timeout, [p, t, this]() {
        p->push(QString("ping-%1 ").arg(count));
        appendTest(QString("ping-%1 ").arg(count));
        ++count;
        if(count > 10) {
            p->push(QString("pong-%1 ").arg(count));
            p->complete();
            t->stop();
        }
    });

    /*
     * So the problem is that when complete happens the delete has has pending requests,
    */
    Axq::create(p)
    .own(t)
    .delay(5000)
    .async(&Axq::Stream::each<QString>, [](const QString & out) { // asynchronous .each<QString>([](const QString& out){
        print(out);
    })
    .onCompleted<QString>([this](const QString & last) {
        print("it is over ", last, "\n");
        appendTest(last);
    })
    .onCompleted([this]() {
        verifyTest();
        next();
        STREAM_CHECK_MEM;
    });
}

void UnitTest::test_request() {
    STREAM_START_MEM;
    expectTest("Seventh Son has a cat");

    Axq::iterator<QString>(QString("Seventh Son has a cat"))
    .request(500)
    .each<QChar>([this](const QChar & c) {
        print(c);
        appendTest(c);
    })  .onCompleted([this]() {
        verifyTest();
        next();
        STREAM_CHECK_MEM;
    });
}

void UnitTest::test_buffer() {
    STREAM_START_MEM;
    expectTest(QString("Moi").repeated(8)); //index starts from zero
    Axq::repeater(100, QString("Moi"))
    .meta<Axq::ParamList, QString, Axq::Stream::Index>([](const QString & s, ulong index) {
        return Axq::params(s, index);
    })
    .completeEach<Axq::ParamList>([](const Axq::ParamList & p) {
        const auto count = Axq::get<1, ulong>(p);
        return count >= 25;
    })
    .map<QString, Axq::ParamList>([](const Axq::ParamList & p) {
        return Axq::get<0, QString>(p);
    })
    .buffer(20)
    .iterate()
    .each<QString>([this](const QString & s) {
        appendTest(s);
        print(s);
    })
    .meta<bool, Axq::Stream::Index>([](ulong index) {
        return index == 7;
    })      .meta<bool, Axq::Stream::This>([](bool v, Axq::Stream s) {
        if(v) {
            s.cancel();
        }
        return v;
    })
    .onError<Axq::Param>([this](const Axq::Param&, int) {
        print("\n");
        verifyTest();
        STREAM_CHECK_MEM;
        next();
    });
}

void UnitTest::test_wait() {
    STREAM_START_MEM;
    QTimer* t = new QTimer();
    expectTest("Wait");
    Axq::repeater(500, QString("Wait"))
    .each<QString>([this](const QString & str) {
        appendTest(str);
    })
    .own(t)
    .wait(t, &QTimer::timeout)
    .completeEach()
    .onCompleted([this] {
        print("\n");
        verifyTest();
        STREAM_CHECK_MEM;
        next();
    });
    t->start(5000);
}


void UnitTest::test_cancel() {
    STREAM_START_MEM;
    expectTest("CancelCancelCancel");
    auto rep = Axq::repeater(500, QString("Cancel"));
    rep.meta<QString, Axq::Stream::Index>([rep, this](const QString & str, ulong index) mutable {
        if(index == 3)
            rep.cancel();
        appendTest(str);
        return str;
    })
    .onCompleted([] {
        Q_ASSERT(false);
    })
    .onError<QString>([this](const QString & s, int e) {
        Q_ASSERT(e == Axq::Stream::StreamCancelValue);
        print("Cancelled", s, "\n");
        verifyTest();
        STREAM_CHECK_MEM;
        next();
    });
}

void UnitTest::test_complete() {
    expectTest("12");
    auto repeater = Axq::repeater<QString>(200, "12")
    .meta<QString, Axq::Stream::This>([this](const QString & t, Axq::Stream s) {
        s.complete();
        appendTest(t);
        return t;
    }).onCompleted([this] {
        print("\n");
        verifyTest();
        next();
    });
}
