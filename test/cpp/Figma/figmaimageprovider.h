#ifndef FIGMAIMAGEPROVIDER_H
#define FIGMAIMAGEPROVIDER_H

#include <QObject>
#include <QQuickAsyncImageProvider>

class FigmaImages;


class FigmaImageProvider : public QObject{
    Q_OBJECT
    Q_PROPERTY(QString userToken MEMBER m_userToken NOTIFY userTokenChanged)
    Q_PROPERTY(QString projectToken MEMBER m_projectToken NOTIFY projectTokenChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString provider MEMBER m_name CONSTANT)
public:
    FigmaImageProvider(QObject* parent = nullptr);
    ~FigmaImageProvider();
    Q_INVOKABLE QString name(int index) const;
    Q_INVOKABLE bool save(int index, const QString& dir) const;
    void addImage(const QByteArray& bytes, const QString& name);
    void load();
    int count() const;
    void set(QQmlEngine& engine);
signals:
    void userTokenChanged();
    void projectTokenChanged();
    void countChanged();
private:
    friend class FigmaImages;
    const QString m_name = "figmaImages";
    FigmaImages* m_provider;
    QVector<QPair<QByteArray,QString>> m_images;
    QString m_userToken;
    QString m_projectToken;
    QNetworkAccessManager* m_access;
};

#endif // FIGMAIMAGEPROVIDER_H
