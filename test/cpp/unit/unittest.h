#ifndef UNITTEST_H
#define UNITTEST_H

#include <functional>
#include <QObject>
#include <QHash>

class Dummy : public QObject {
    Q_OBJECT
public:
    Dummy(std::function<void ()> onDelete) : m_onDelete(onDelete) {}
    ~Dummy() {m_onDelete();}
    std::function<void ()> m_onDelete;
};

class TestData : public QObject {
    Q_OBJECT
public:
    TestData(QObject* parent) : QObject(parent) {}
    void setBrief(const QString& name, const QString& brief) {
        if(!hasBrief(name)) {
            m_doxTree.insert(name, QHash<QString, QString>({{"brief", brief}}));
        } else {
            append(m_doxTree[name]["brief"], brief);
        }
    }
    void setData(const QString& name, const QString& key, const QString& data) {
        Q_ASSERT(hasBrief(name));
        if(!m_doxTree[name].contains(key)) {
            m_doxTree[name].insert(key, data);
        } else {
            append(m_doxTree[name][key], data);
        }
    }
    bool hasBrief(const QString& name) const {return m_doxTree.contains(name);}
    QStringList briefs() const {return m_doxTree.keys();}
    QString getData(const QString& name, const QString& key) const {return m_doxTree[name][key];}
    QStringList datum(const QString& name) const {return m_doxTree[name].keys();}
private:
    void append(QString& s, const QString& txt) {
        s += s.length() > 0 && s.at(s.length() - 1).isLetterOrNumber() ? (" " + txt) : txt;
    }
private:
    QHash<QString, QHash<QString, QString>> m_doxTree;
};

class UnitTest : public QObject {
    Q_OBJECT
public:
    UnitTest();
    void start();
    void readDox();
private:
    template <typename T>
    void appendTest(T p0) {
        m_buffer.append(QString("%1").arg(p0));
    }

    template <typename T, typename ...Args>
    void appendTest(T p0, Args... args) {
        m_buffer.append(QString("%1").arg(p0));
        appendTest(args...);
    }

    template <typename iterator>
    void appendTest(iterator begin, iterator end, const QString& sep = ",") {
        if(begin != end) {
            auto it = begin;
            auto last = end - 1;
            for(; it != last; ++it) {
                appendTest(*it);
                appendTest(sep);
            }
            if(it != end) {
                appendTest(*it);
            }
        }
    }

    template <typename iterator>
    void expectTest(iterator begin, iterator end, const QString& sep = ",") {
        appendTest(begin, end, sep); //using expectTest side effect that clear m_buffer
        auto s = m_buffer.join("");
        expectTest(s);
    }
    void expectTest(const QString& expectation);
    void verifyTest(std::function<bool (const QString&, const QString&)> match = nullptr);
signals:
    void next();
    void exit();
    void doxRead();
public slots:
    void test_merge();
    void test_sleepSort();
    void test_sleepSort2();
    void test_scan();
    void test_erastothenes();
    void test_async();
    void test_defer();
    void test_checkApi();
    void test_split();
    void test_error();
    void test_flush();
    void test_buffer();
    void test_request();
    void test_wait();
    void test_cancel();
    void test_complete();
private:
    const int m_testCount;
    int m_currentTest = 0;
    int m_methodIndex = 0;
    int m_currentLine = -1; //used to know if current has been attempt to verify
    int m_currentMethod = -1;
    TestData* m_dox = nullptr;
    QStringList m_buffer;
};

#endif // UNITTEST_H
