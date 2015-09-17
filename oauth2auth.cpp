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
along with hangish.  If not, see <http://www.gnu.org/licenses/>

*/

#include "oauth2auth.h"
#include <QtWidgets/QApplication>
#include <QProcess>
static QString user_agent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/34.0.1847.132 Safari/537.36";
QString homeDir;
QString homePath;

static QString OAUTH2_LOGIN_URL="https://accounts.google.com/o/oauth2/auth?scope=https://www.google.com/accounts/OAuthLogin%20https://www.googleapis.com/auth/userinfo.email&redirect_uri=urn:ietf:wg:oauth:2.0:oob&response_type=code&client_id=936475272427.apps.googleusercontent.com";
static QString CLIENT_ID = "936475272427.apps.googleusercontent.com";
static QString CLIENT_SECRET = "KWsJlkaMn1jGLxQpWxMnOox-";

OAuth2Auth::OAuth2Auth()
{
    homeDir  = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    homePath = homeDir + "/oauth2.json";
    //qDebug() << "Making dir " << homeDir;
    QDir().mkpath(homeDir);
}

bool OAuth2Auth::getSavedToken()
{
    QFile cookieFile(homePath);
    cookieFile.open(QIODevice::ReadOnly | QIODevice::Text);
    if (!cookieFile.exists())
        return false;
    QString val = cookieFile.readAll();
    cookieFile.close();
    QJsonDocument doc = QJsonDocument::fromJson(val.toUtf8());
    QJsonObject obj = doc.object();
    //foreach (QString s, obj.keys())
    //{
    //    qDebug() << s;
    //}
    rtoken = obj["refresh_token"].toString();
    //REFRESH TOKEN
    //fetchCookies(obj["access_token"].toString(), obj["refresh_token"].toString());
    QString r = QString("refresh_token=" + obj["refresh_token"].toString() + "&client_id="+CLIENT_ID);
    r+=QString("&client_secret="+CLIENT_SECRET);
    r+="&grant_type=refresh_token";
    //qDebug() << r;
    QByteArray reqString(r.toUtf8());
    QNetworkRequest req( QUrl( QString("https://accounts.google.com/o/oauth2/token") ) );
    req.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    QNetworkReply *reply = nam.post(req, reqString);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(authReply()));
}

void OAuth2Auth::fetchCookiesReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    //qDebug() << "Got " << c.size() << "from" << reply->url();
    //foreach(QNetworkCookie cookie, c) {
    //    qDebug() << cookie.name();
    //}
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200 || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==302) {
        //GET REFRESH TOKEN
        QString response = reply->readAll();
        //qDebug() << response;

        //MAKE REQ
        QNetworkRequest req2( QUrl("https://accounts.google.com/MergeSession?service=mail&continue=http://www.google.com&uberauth="+response) );
        req2.setRawHeader("Authorization", auth_header.toLocal8Bit().data() );
        req2.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(sessionCookies));
        QNetworkReply *reply2 = nam.get(req2);
        QObject::connect(reply2, SIGNAL(finished()), this, SLOT(fetchCookiesReply3()));
    }
    else {
        emit authFailed("Error 1");
    }
}

void OAuth2Auth::fetchCookiesReply2()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    //qDebug() << "Got " << c.size() << "from" << reply->url();
    //foreach(QNetworkCookie cookie, c) {
    //    qDebug() << cookie.name();
    //}
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200 || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==302) {
        //GET REFRESH TOKEN
        //QString response = reply->readAll();
        //qDebug() << response;
    }
    else {
        emit authFailed("Error 2");
    }
}

void OAuth2Auth::fetchCookiesReply3()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    //qDebug() << "Got " << c.size() << "from" << reply->url();
    foreach(QNetworkCookie cookie, c) {
        //qDebug() << cookie.name();
        sessionCookies.append(cookie);
    }
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200 || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==302) {
        //GET REFRESH TOKEN
        QString response = reply->readAll();
        //qDebug() << response;
        emit gotCookies();
    }
    else {
        //qDebug() << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        emit authFailed("Error 3");
    }
}


void OAuth2Auth::fetchCookies(QString access_token, QString refresh_token)
{
    QNetworkRequest req( QUrl("https://accounts.google.com/accounts/OAuthLogin?source=hangish&issueuberauth=1") );
    auth_header = "Bearer " + access_token;
    req.setRawHeader("Authorization", auth_header.toLocal8Bit().data() );
    req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(sessionCookies));
    QNetworkReply *reply = nam.get(req);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(fetchCookiesReply()));
}

void OAuth2Auth::sendAuthRequest(QString code)
{
    QString r = QString("code=" + code + "&client_id="+CLIENT_ID);
    r+=QString("&client_secret="+CLIENT_SECRET);
    r+="&grant_type=authorization_code&redirect_uri=urn:ietf:wg:oauth:2.0:oob";
    QByteArray reqString(r.toUtf8());
    QNetworkRequest req( QUrl( QString("https://accounts.google.com/o/oauth2/token") ) );
    req.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    QNetworkReply *reply = nam.post(req, reqString);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(authReply()));
}

void OAuth2Auth::followRedirection(QUrl url)
{
    QNetworkRequest req( url );
    req.setRawHeader("User-Agent", user_agent.toLocal8Bit().data());
    //req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(sessionCookies));
    nam.get(req);
}

void OAuth2Auth::authReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    //qDebug() << "Got " << c.size() << "from" << reply->url();
    QString response = reply->readAll();
    //qDebug() << response;
    //foreach(QNetworkCookie cookie, c) {
    //    qDebug() << cookie.name();
    //}
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200) {
        //GET REFRESH TOKEN
        QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8());
        QJsonObject obj = doc.object();
        //foreach (QString s, obj.keys())
        //{
        //    qDebug() << s;
        //}
        if (rtoken.length() > 0)
            obj.insert("refresh_token", rtoken);
        QJsonDocument doc2(obj);
        //qDebug() << obj.take("refresh_token");

        //SAVE TOKEN
        QFile cookieFile(homePath);
        if (cookieFile.exists())
            cookieFile.remove();

        if ( cookieFile.open(QIODevice::WriteOnly))
                {
            cookieFile.write(doc2.toJson(), doc2.toJson().size());
            }

        cookieFile.close();

        qDebug() << "Written cookies to " << homePath;

        //MAKE NEW REQ
        fetchCookies(obj["access_token"].toString(), obj["refresh_token"].toString());
        //headers = { 'Authorization':'Bearer {0}'.format(js['access_token']) }
        //r = session.get('https://accounts.google.com/accounts/OAuthLogin?source=hangish&issueuberauth=1', headers=headers)
    }
    else {
        emit authFailed("No network");
    }
}

QList<QNetworkCookie> OAuth2Auth::getCookies()
{
    return sessionCookies;
}

void OAuth2Auth::updateCookies(QList<QNetworkCookie> cookies)
{
    sessionCookies.clear();
    foreach(QNetworkCookie cookie, cookies) {
            sessionCookies.append(cookie);
    }
    emit(gotCookies());
}

void OAuth2Auth::auth()
{
    if (!getSavedToken())
        //open html page
        emit loginNeeded();
}
