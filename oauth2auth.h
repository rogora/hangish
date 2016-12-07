#ifndef OAUTH2AUTH_H
#define OAUTH2AUTH_H

#include "qtimports.h"

class OAuth2Auth: public QObject
{
    Q_OBJECT

private:
    QNetworkAccessManager *nam;
    QList<QNetworkCookie> sessionCookies;
    void followRedirection(QNetworkReply *reply, int caller);
    void fetchCookies(QString access_token);
    QString rtoken;
    void sendUsername(QString username);
    QString gxf, TL, challengeType, challengeId;
    QString prInfo, sessState, contString;
    QString email, password;
    bool loginNeededVar;

public:
    QString auth_header;
    OAuth2Auth();
    bool getSavedToken();
    void auth();
    void sendAuthRequest(QString code);
    QList<QNetworkCookie> getCookies();
    void updateCookies(QList<QNetworkCookie> cookies);
    void openLoginPage(QString uname, QString pwd);
    void sendPassword(QString uname, QString password);
    void sendPin(QString pin);
    bool isLoginNeeded();

signals:
    void loginNeeded();
    void gotCookies();
    void authFailed(QString error);
    void secondFactorNeeded();

public slots:
    void netError(QNetworkReply::NetworkError err);
    void authReply();
    void fetchCookiesReply();
    void fetchCookiesReply2();
    void fetchCookiesReply3();

    void loginpageReply();
    void unameReply();
    void pwdReply();
    void pinReply();

};

#endif // OAUTH2AUTH_H
