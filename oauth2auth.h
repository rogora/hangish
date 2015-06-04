#ifndef OAUTH2AUTH_H
#define OAUTH2AUTH_H

#include "qtimports.h"

class OAuth2Auth: public QObject
{
    Q_OBJECT

private:
    QNetworkAccessManager nam;
    QList<QNetworkCookie> sessionCookies;
    void followRedirection(QUrl url);
    void fetchCookies(QString access_token, QString refresh_token);
    QString rtoken;

public:
    QString auth_header;
    OAuth2Auth();
    bool getSavedToken();
    void auth();
    void sendAuthRequest(QString code);
    QList<QNetworkCookie> getCookies();
    void updateCookies(QList<QNetworkCookie> cookies);


signals:
    void loginNeeded();
    void gotCookies();
    void authFailed(QString error);

public slots:
    void authReply();
    void fetchCookiesReply();
    void fetchCookiesReply2();
    void fetchCookiesReply3();

};

#endif // OAUTH2AUTH_H
