/*

Hanghish
Copyright (C) 2015 Daniele Rogora

This file is part of Hangish.

Hangish is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Hangish is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Nome-Programma.  If not, see <http://www.gnu.org/licenses/>

*/


#ifndef AUTHENTICATOR_H
#define AUTHENTICATOR_H

#include "qtimports.h"

class Authenticator : public QObject
{
    Q_OBJECT
    QList<QNetworkCookie> sessionCookies;
    QList<QNetworkCookie> liveSessionCookies;

    QNetworkAccessManager nam;
    QNetworkCookieJar cJar;
    QNetworkCookie GALXCookie;
    QUrl redirection;
    int auth_phase;

public:
    void updateCookies(QList<QNetworkCookie> cookies);
    Authenticator();
    void auth();
    QList<QNetworkCookie> getCookies();
    void send_credentials(QString uname, QString passwd);
    void send2ndFactorPin(QString pin);

private:
    QString secTok, timeStmp;
    void getGalxToken();
    void getAuthCookies();
    void followRedirection(QUrl url);
    bool amILoggedIn();

public slots:
    void cb(QNetworkReply *reply);

signals:
    void loginNeeded();
    void gotCookies();
    void authFailed(QString error);
};

#endif // AUTHENTICATOR_H
