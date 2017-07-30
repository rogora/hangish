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
static QString user_agent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.100 Safari/537.36";
QString homeDir;
QString homePath;

static QString CLIENT_ID = "936475272427.apps.googleusercontent.com";
static QString CLIENT_SECRET = "KWsJlkaMn1jGLxQpWxMnOox-";
static QString LOGIN_URL = "https://accounts.google.com/o/oauth2/programmatic_auth?client_id=" + QUrl::toPercentEncoding("936475272427.apps.googleusercontent.com") + "&scope=" + QUrl::toPercentEncoding("https://www.google.com/accounts/OAuthLogin") + "+" + QUrl::toPercentEncoding("https://www.googleapis.com/auth/userinfo.email");

OAuth2Auth::OAuth2Auth()
{
    nam = new QNetworkAccessManager();
    loginNeededVar = false;
    homeDir  = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    homePath = homeDir + "/oauth2.json";
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
    rtoken = obj["refresh_token"].toString();
    //REFRESH TOKEN
    QString r = QString("refresh_token=" + obj["refresh_token"].toString() + "&client_id="+CLIENT_ID);
    r+=QString("&client_secret="+CLIENT_SECRET);
    r+="&grant_type=refresh_token";
    //qDebug() << r;
    QByteArray reqString(r.toUtf8());
    QNetworkRequest req( QUrl( QString("https://accounts.google.com/o/oauth2/token") ) );
    req.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    QNetworkReply *reply = nam->post(req, reqString);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(authReply()));
    QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(netError(QNetworkReply::NetworkError)));
    return true;
}

void OAuth2Auth::fetchCookiesReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200 || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==302) {
        //GET REFRESH TOKEN
        QString response = reply->readAll();
        qDebug() << response;

        // MAKE REQ
        QNetworkRequest req2( QUrl("https://accounts.google.com/MergeSession?service=mail&continue=http://www.google.com&uberauth="+response) );
        req2.setRawHeader("Authorization", auth_header.toLocal8Bit().data() );
        req2.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(sessionCookies));
        QNetworkReply *reply2 = nam->get(req2);
        QObject::connect(reply2, SIGNAL(finished()), this, SLOT(fetchCookiesReply3()));
        QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(netError(QNetworkReply::NetworkError)));
    }
    else {
        emit authFailed("Error 1");
    }
    reply->deleteLater();
}

void OAuth2Auth::fetchCookiesReply2()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200 || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==302) {
        // Nothing to do here
    }
    else {
        emit authFailed("Error 2 " + reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
    }
    reply->deleteLater();
}

void OAuth2Auth::fetchCookiesReply3()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    foreach(QNetworkCookie cookie, c) {
        sessionCookies.append(cookie);
    }
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200 || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==302) {
        //GET REFRESH TOKEN
        QString response = reply->readAll();
        qDebug() << response;
        emit gotCookies();
    }
    else {
        //qDebug() << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        emit authFailed("Error 3 " + reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
    }
    reply->deleteLater();
}


void OAuth2Auth::fetchCookies(QString access_token)
{
    QNetworkRequest req( QUrl("https://accounts.google.com/accounts/OAuthLogin?source=hangish&issueuberauth=1") );
    auth_header = "Bearer " + access_token;
    req.setRawHeader("Authorization", auth_header.toLocal8Bit().data() );
    req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(sessionCookies));
    QNetworkReply *reply = nam->get(req);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(fetchCookiesReply()));
    QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(netError(QNetworkReply::NetworkError)));
}

void OAuth2Auth::sendAuthRequest(QString code)
{
    QString r = QString("code=" + code + "&client_id="+CLIENT_ID);
    r+=QString("&client_secret="+CLIENT_SECRET);
    r+="&grant_type=authorization_code&redirect_uri=urn:ietf:wg:oauth:2.0:oob";
    QByteArray reqString(r.toUtf8());
    QNetworkRequest req( QUrl( QString("https://accounts.google.com/o/oauth2/token") ) );
    req.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    QNetworkReply *reply = nam->post(req, reqString);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(authReply()));
    QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(netError(QNetworkReply::NetworkError)));
}

void OAuth2Auth::followRedirection(QNetworkReply *reply, int caller)
{
    QString redir = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toString();
    QVariant vredir(redir);
    redir = QUrl::fromPercentEncoding(vredir.toByteArray());
    qDebug() << redir;
    if (redir == "https://accounts.google.com/o/oauth2/programmatic_auth?client_id=936475272427.apps.googleusercontent.com") {
    //GET REFRESH TOKEN
    //if (reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toString().contains(CLIENT_ID)) {
        QNetworkRequest req(LOGIN_URL);
        req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(sessionCookies));
        req.setRawHeader("User-Agent", user_agent.toLocal8Bit().data());
        req.setRawHeader("Upgrade-Insecure-Requests", "1");
        req.setRawHeader("Host", "accounts.google.com");
        req.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
        req.setRawHeader("Referer", "https://accounts.google.com/AccountLoginInfo");
        QNetworkReply *reply = nam->get(req);
        QObject::connect(reply, SIGNAL(finished()), this, SLOT(pinReply()));
        QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(netError(QNetworkReply::NetworkError)));
    }
    else {
        QNetworkRequest req(redir);
        req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(sessionCookies));
        req.setRawHeader("User-Agent", user_agent.toLocal8Bit().data());
        req.setRawHeader("Upgrade-Insecure-Requests", "1");
        req.setRawHeader("Host", "accounts.google.com");
        req.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
        req.setRawHeader("Referer", "https://accounts.google.com/AccountLoginInfo");

        QNetworkReply *reply = nam->get(req);
        if (caller == 0)
            QObject::connect(reply, SIGNAL(finished()), this, SLOT(pwdReply()));
        else if (caller == 1)
            QObject::connect(reply, SIGNAL(finished()), this, SLOT(pinReply()));
        QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(netError(QNetworkReply::NetworkError)));
    }
    reply->deleteLater();
}

void OAuth2Auth::authReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    QString response = reply->readAll();
    //qDebug() << response;

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200) {
        //GET REFRESH TOKEN
        QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8());
        QJsonObject obj = doc.object();

        if (rtoken.length() > 0)
            obj.insert("refresh_token", rtoken);
        QJsonDocument doc2(obj);

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
        fetchCookies(obj["access_token"].toString());
    }
    else {
        emit authFailed("No network");
    }
    reply->deleteLater();
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

bool OAuth2Auth::isLoginNeeded()
{
    return loginNeededVar;
}

void OAuth2Auth::auth()
{
    if (!getSavedToken()) {
            loginNeededVar = true;
            emit loginNeeded();
        }
}

void OAuth2Auth::openLoginPage(QString uname, QString pwd)
{
    email = uname;
    password = pwd;
    QString url = LOGIN_URL;
    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", user_agent.toLocal8Bit().data());
    //req.setRawHeader("Accept-Encoding", "gzip, deflate, br");
    req.setRawHeader("Upgrade-Insecure-Requests", "1");
    req.setRawHeader("Host", "accounts.google.com");
    req.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");

    QNetworkReply *reply = nam->get(req);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(loginpageReply()));
    QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(netError(QNetworkReply::NetworkError)));
}

void OAuth2Auth::loginpageReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    foreach(QNetworkCookie cookie, c) {
        qDebug() << cookie.name();
        sessionCookies.append(cookie);
    }
    QString response = reply->readAll();
    //qDebug() << response;

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==302) {
        //GET REFRESH TOKEN
        QNetworkRequest req(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl());
        req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(sessionCookies));
        QNetworkReply *reply = nam->get(req);
        QObject::connect(reply, SIGNAL(finished()), this, SLOT(loginpageReply()));
        QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(netError(QNetworkReply::NetworkError)));
    }
    else if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200) {
        // get gxf
        int gxfi = response.indexOf("name=\"gxf\"");
        if (gxfi > 0) {
            gxfi = response.indexOf("value=\"", gxfi) + strlen("value=\"");
            this->gxf = response.mid(gxfi, response.indexOf("\"", gxfi + 1) - gxfi);
            qDebug() << "gxf: " << this->gxf;
        }
        int cni = response.indexOf("name=\"continue\"");
        if (cni > 0) {
            cni = response.indexOf("value=\"", cni) + strlen("value=\"");
            contString = response.mid(cni, response.indexOf("\"", cni + 1) - cni);
        }
        sendUsername(email);
    }
    else {
        emit authFailed("Error L1");
    }
    reply->deleteLater();
}

void OAuth2Auth::sendUsername(QString username)
{
    QNetworkRequest req(QUrl("https://accounts.google.com/_/signin/v1/lookup"));
    QString GALX = "";
    foreach (QNetworkCookie c, sessionCookies) {
        qDebug() << c.name() << ": " << c.value();
        if (c.name() == "GALX")
            GALX = c.value();
    }
    req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(sessionCookies));
    req.setRawHeader("User-Agent", user_agent.toLocal8Bit().data());
    req.setRawHeader("Host", "accounts.google.com");
    //req.setRawHeader("Accept-Encoding", "gzip, deflate, br");
    req.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
    req.setRawHeader("Referer", "https://accounts.google.com/AccountLoginInfo");
    req.setRawHeader("Content-length", "548");
    req.setRawHeader("Accept-Language", "en");
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QString r = "gxf=" + QUrl::toPercentEncoding(gxf) + "&GALX=" + QUrl::toPercentEncoding(GALX) + "&Email=" + username + "&oauth=1&signIn=Next&Page=PasswordSeparationSignIn&PersistentCookie=yes&ltmpl=embedded&oauth=1&pstMsg=1&sarp=1&scc=1&continue=" + QUrl::toPercentEncoding(contString);
    //qDebug() << r;
    QByteArray reqString(r.toUtf8());
    QNetworkReply *reply = nam->post(req, reqString);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(unameReply()));
    QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(netError(QNetworkReply::NetworkError)));
}

void OAuth2Auth::unameReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    foreach(QNetworkCookie cookie, c) {
        qDebug() << cookie.name();
        if (cookie.name() == "GAPS") {
            // find and remove GAPS
            int co;
            for (co = 0; co < sessionCookies.size(); co++)
                if (sessionCookies[co].name() == "GAPS")
                    break;
            sessionCookies.removeAt(co);
        }
        sessionCookies.append(cookie);
    }

    qDebug() << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QString response = reply->readAll();
    qDebug() << response;

    //qDebug() << "Got " << c.size() << "from" << reply->url();
    //foreach(QNetworkCookie cookie, c) {
    //    qDebug() << cookie.name();
    //}
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==302) {
        //GET REFRESH TOKEN
        QNetworkRequest req(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl());
        req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(sessionCookies));
        req.setRawHeader("User-Agent", user_agent.toLocal8Bit().data());
        req.setRawHeader("Host", "accounts.google.com");
        req.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
        req.setRawHeader("Referer", "https://accounts.google.com/AccountLoginInfo");

        QNetworkReply *reply = nam->get(req);
        QObject::connect(reply, SIGNAL(finished()), this, SLOT(unameReply()));
        QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(netError(QNetworkReply::NetworkError)));
    }
    else if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200) {
        // get gxf and update
        int gxfi = response.indexOf("name=\"gxf\"");
        if (gxfi > 0) {
            gxfi = response.indexOf("value=\"", gxfi) + strlen("value=\"");
            this->gxf = response.mid(gxfi, response.indexOf("\"", gxfi + 1) - gxfi);
            qDebug() << "gxf: " << this->gxf;
        }
        // get profile-information
        int pii = response.indexOf("name=\"ProfileInformation\"");
        if (pii > 0) {
            pii = response.indexOf("value=\"", pii) + strlen("value=\"");
            prInfo = response.mid(pii, response.indexOf("\"", pii + 1) - pii);
        }
        int ssi = response.indexOf("name=\"SessionState\"");
        if (ssi > 0) {
            ssi = response.indexOf("value=\"", ssi) + strlen("value=\"");
            sessState = response.mid(ssi, response.indexOf("\"", ssi + 1) - ssi);
        }
        int cni = response.indexOf("name=\"continue\"");
        if (cni > 0) {
            cni = response.indexOf("value=\"", cni) + strlen("value=\"");
            contString = response.mid(cni, response.indexOf("\"", cni + 1) - cni);
        }
        sendPassword(email, password);

    }
    else {
        emit authFailed("Error L2");
    }
    reply->deleteLater();
}

void OAuth2Auth::sendPassword(QString uname, QString password)
{
    qDebug("Sending password!");
    //qDebug() << sessState;
    //qDebug() << prInfo;
    qDebug() << contString;
    qDebug() << QUrl::toPercentEncoding(contString);
    QString GALX = "";
    QNetworkRequest req(QUrl("https://accounts.google.com/signin/challenge/sl/password"));
    foreach (QNetworkCookie c, sessionCookies) {
        qDebug() << c.name() << ": " << c.value();
        if (c.name() == "GALX")
            GALX = c.value();
    }
    req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(sessionCookies));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    req.setRawHeader("User-Agent", user_agent.toLocal8Bit().data());
    //req.setRawHeader("Accept-Encoding", "gzip, deflate, br");
    req.setRawHeader("Upgrade-Insecure-Requests", "1");
    req.setRawHeader("Host", "accounts.google.com");
    req.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
    req.setRawHeader("Referer", "https://accounts.google.com/AccountLoginInfo");
    QString r = "gxf=" + QUrl::toPercentEncoding(gxf) + "&GALX=" + GALX + "&Email=" + uname + "&Passwd=" + password + "&ProfileInformation=" + QUrl::toPercentEncoding(prInfo) + "&SessionState=" + QUrl::toPercentEncoding(sessState) + "&continue=" + QUrl::toPercentEncoding(contString) + "&signIn=Sign+in&ltmpl=embedded&oauth=1&pstMsg=1&checkedDomains=youtube&bgresponse=js_disabled&sarp=1&scc=1&Page=PasswordSeparationSignIn&identifier-captcha-input=&identifiertoken=&identifiertoken_audio=&PersistentCookie=yes";
    QByteArray reqString(r.toUtf8());
    QNetworkReply *reply = nam->post(req, reqString);
    //qDebug() << r;
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(pwdReply()));
    QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(netError(QNetworkReply::NetworkError)));
}

void OAuth2Auth::pwdReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    foreach(QNetworkCookie cookie, c) {
        qDebug() << cookie.name();
        if (cookie.name() == "GAPS") {
            // find and remove GAPS
            int co;
            for (co = 0; co < sessionCookies.size(); co++)
                if (sessionCookies[co].name() == "GAPS")
                    break;
            sessionCookies.removeAt(co);
        }
        else if (cookie.name() == "oauth_code") {
            qDebug() << "We're done, challenge passed";
            sessionCookies.clear();
            nam->clearAccessCache();
            sendAuthRequest(cookie.value());
            reply->deleteLater();
            return;
        }
        sessionCookies.append(cookie);
    }


    qDebug() << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() << " - " << reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toString();
    QString response = reply->readAll();
    qDebug() << response;

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==302) {
        followRedirection(reply, 0);
    }
    else if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200) {
        foreach(QNetworkCookie cookie, c) {
                if (cookie.name() == "oauth_code") {
                    qDebug() << "We're done, no challenge";
                    sendAuthRequest(cookie.value());
                    return;
                }
        }
        if (!reply->url().toString().contains("challenge")) {
            // no code and no challenge. It likely failed
            emit authFailed("Wrong username/password");
            return;
        }
        // if we're here we have to complete the 2nd challenge
        emit secondFactorNeeded();

        // get gxf and update
        int gxfi = response.indexOf("name=\"gxf\"");
        if (gxfi > 0) {
            gxfi = response.indexOf("value=\"", gxfi) + strlen("value=\"");
            this->gxf = response.mid(gxfi, response.indexOf("\"", gxfi + 1) - gxfi);
            qDebug() << "gxf: " << this->gxf;
        }
        // get profile-information
        int pii = response.indexOf("name=\"challengeId\"");
        if (pii > 0) {
            pii = response.indexOf("value=\"", pii) + strlen("value=\"");
            challengeId = response.mid(pii, response.indexOf("\"", pii + 1) - pii);
        }
        int ssi = response.indexOf("name=\"challengeType\"");
        if (ssi > 0) {
            ssi = response.indexOf("value=\"", ssi) + strlen("value=\"");
            challengeType = response.mid(ssi, response.indexOf("\"", ssi + 1) - ssi);
        }
        int cni = response.indexOf("name=\"continue\"");
        if (cni > 0) {
            cni = response.indexOf("value=\"", cni) + strlen("value=\"");
            contString = response.mid(cni, response.indexOf("\"", cni + 1) - cni);
        }
        int tli = response.indexOf("name=\"TL\"");
        if (tli > 0) {
            tli = response.indexOf("value=\"", tli) + strlen("value=\"");
            TL = response.mid(tli, response.indexOf("\"", tli + 1) - tli);
        }
    }
    else {
        emit authFailed("Error L3 " + reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
    }
    reply->deleteLater();
}

void OAuth2Auth::sendPin(QString pin)
{
    QNetworkRequest req(QUrl("https://accounts.google.com/signin/challenge/ipp/2"));
    req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(sessionCookies));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    req.setRawHeader("User-Agent", user_agent.toLocal8Bit().data());
    req.setRawHeader("Upgrade-Insecure-Requests", "1");
    req.setRawHeader("Host", "accounts.google.com");
    req.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
    req.setRawHeader("Referer", "https://accounts.google.com/AccountLoginInfo");
    QString r = "Pin=" + pin + "&gxf=" + QUrl::toPercentEncoding(gxf) + "&continue=" + QUrl::toPercentEncoding(contString) + "&TL=" + QUrl::toPercentEncoding(TL) + "&TrustDevice=on&challengeId=" + challengeId + "&challengeType=" + challengeType + "&pstMsg=1&sarp=1&scc=1";
    QByteArray reqString(r.toUtf8());
    QNetworkReply *reply = nam->post(req, reqString);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(pinReply()));
    QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(netError(QNetworkReply::NetworkError)));
}

void OAuth2Auth::pinReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    foreach(QNetworkCookie cookie, c) {
        if (cookie.name() == "GAPS") {
            // find and remove GAPS
            int co;
            for (co = 0; co < sessionCookies.size(); co++)
                if (sessionCookies[co].name() == "GAPS")
                    break;
            sessionCookies.removeAt(co);
        }
        qDebug() << cookie.name();
        sessionCookies.append(cookie);

        if (cookie.name() == "oauth_code") {
            qDebug() << "We're done, challenge passed";
            sessionCookies.clear();
            nam->clearAccessCache();
            sendAuthRequest(cookie.value());
            reply->deleteLater();
            return;
        }
    }

    QString response = reply->readAll();
    qDebug() << response;

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==302) {
        followRedirection(reply, 1);
    }
    else if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200) {
        foreach(QNetworkCookie cookie, c) {
                if (cookie.name() == "oauth_code") {
                    qDebug() << "We're done, challenge passed";
                    //sessionCookies.clear();
                    password = email = "";
                    sendAuthRequest(cookie.value());
                }
        }
    }
    else {
        emit authFailed("Error L4");
    }
    reply->deleteLater();
}

void OAuth2Auth::netError(QNetworkReply::NetworkError err) {
    qDebug() << "Error in authenticator " << err;
    nam->deleteLater();
    nam = new QNetworkAccessManager();
}
