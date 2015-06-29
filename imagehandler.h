#ifndef IMAGEHANDLER_H
#define IMAGEHANDLER_H

#include <QObject>
#include "qtimports.h"
#include "oauth2auth.h"

class ImageHandler : public QObject
{
    Q_OBJECT

private:
    QNetworkAccessManager *nam;
    QByteArray buffer;
    QAtomicInt lock;
    OAuth2Auth *auth;

public:
    explicit ImageHandler(QObject *parent = 0);
    Q_INVOKABLE QString getImageAddr(QString imgUrl);
    Q_INVOKABLE bool saveImageToGallery(QString imgUrl);
    void cleanCache();
    void setAuthenticator(OAuth2Auth *pAuth);

signals:
    void imgReady(QString path);
    void savedToGallery();

public slots:

private slots:
    void gotImage();
    void dataAvail();

};

#endif // IMAGEHANDLER_H
