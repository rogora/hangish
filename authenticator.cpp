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


#include "authenticator.h"
#include <QtWidgets/QApplication>
#include <QProcess>
static QString user_agent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/34.0.1847.132 Safari/537.36";
static QString homePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/cookies.json";

static QString SECONDFACTOR_URL = "https://accounts.google.com/SecondFactor";

void Authenticator::cb(QNetworkReply *reply) {
        QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
        QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
        qDebug() << "Got " << c.size() << "from" << reply->url();
            sessionCookies.append(c);

            if (reply->error() == QNetworkReply::NoError) {
        if (auth_phase==1) {
            QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
            QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
            if (c.at(1).name()=="GALX") {
                        GALXCookie = c.at(1);
                        delete reply;
                        //send_credentials(GALXCookie);
                        }
        }
        else if (auth_phase==2) {
            if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==302) {
                QVariant possibleRedirectUrl =
                             reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

                followRedirection(possibleRedirectUrl.toUrl());
            }
            else if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200) {
                if (amILoggedIn())
                    getAuthCookies();
                else if (reply->url().toString().startsWith(SECONDFACTOR_URL)) {
                    //2nd factor auth
                    qDebug() << "Auth failed " << reply->url();
                    QString ssreply = reply->readAll();

                    //find secTok
                    int start = ssreply.indexOf("id=\"secTok\"");
                    start = ssreply.indexOf("'", start)+1;
                    int stop = ssreply.indexOf("'", start);
                    secTok = ssreply.mid(start, stop-start);

                    //find timeStmp
                    start = ssreply.indexOf("id=\"timeStmp\"");
                    start = ssreply.indexOf("'", start)+1;
                    stop = ssreply.indexOf("'", start);
                    timeStmp = ssreply.mid(start, stop-start);
                    qDebug() << ssreply;
                    qDebug() << secTok;
                    qDebug() << timeStmp;
                    emit authFailed("2nd factor needed");
                }
                else {
                    //Something went wrong
                    qDebug() << "Auth failed " << reply->url();
                    emit authFailed("Wrong uname/passwd");
                }
            }
            else {
                //Something went wrong
                qDebug() << "Auth failed " << reply->errorString();
                emit authFailed(reply->errorString());
            }
        }
        else if (auth_phase==6) {
            QString ssreply = reply->readAll();
            qDebug() << ssreply;
            //2nd factor response
            if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200) {
                //TODO: is this really a consistent check for everyone?
                if (amILoggedIn())
                    getAuthCookies();
                else {
                    emit authFailed(reply->url().toString());
                }
            }
            else if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==302) {
                QVariant possibleRedirectUrl =
                             reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

                followRedirection(possibleRedirectUrl.toUrl());
            }
            else {
                qDebug() << "2nd factor error";
                emit authFailed("Error sending pin");
            }
        }
    }
    else {
        //failure
        delete reply;
    }

}

void Authenticator::followRedirection(QUrl url)
{
    QNetworkRequest req( url );
    req.setRawHeader("User-Agent", user_agent.toLocal8Bit().data());
    req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(sessionCookies));
    nam.get(req);
}

void Authenticator::getGalxToken()
{
    auth_phase = 1;
    QNetworkRequest req( QUrl( QString("https://accounts.google.com/ServiceLogin?passive=true&skipvpage=true&continue=https://talkgadget.google.com/talkgadget/gauth?verify%3Dtrue&authuser=0") ) );
    req.setRawHeader("User-Agent", user_agent.toLocal8Bit().data());
    nam.get(req);
}

void Authenticator::getAuthCookies()
{
    auth_phase = 3;
    QJsonObject obj;
    foreach(QNetworkCookie cookie, sessionCookies) {
        //Let's save the relevant cookies
        if (cookie.name()=="S" || !cookie.isSessionCookie() && cookie.domain().contains("google.com") ) {
            qDebug() << "GOOD: " << cookie.name() << " - " << cookie.domain() << " - " << cookie.isSessionCookie() << " - " << cookie.expirationDate().toString();
            obj[cookie.name()] = QJsonValue( QString(cookie.value()) );
            liveSessionCookies.append(cookie);
        }
    }
    QJsonDocument doc(obj);
    QFile cookieFile(homePath);
    if ( cookieFile.open(QIODevice::WriteOnly))
            {
        cookieFile.write(doc.toJson(), doc.toJson().size());
        }

  cookieFile.close();

    qDebug() << "Written cookies to " << homePath;
    auth_phase=5;
    sessionCookies = liveSessionCookies;
    liveSessionCookies.clear();
    emit(gotCookies());
}

void Authenticator::send_credentials(QString uname, QString passwd)
{
    auth_phase = 2;
    QString r = QString("GALX="+QString(GALXCookie.value()));
    r+=QString("&Email="+uname);
    r+=QString("&Passwd="+passwd);
    r+="&bgresponse=js_disabled&dnConn=0&signIn=Accedi&checkedDomains=youtube&PersistentCookie=yes&rmShown=1&pstMsg=0&skipvpage=true&continue=https://talkgadget.google.com/talkgadget/gauth?verify=true";
    QByteArray reqString(r.toUtf8());
    QNetworkRequest req( QUrl( QString("https://accounts.google.com/ServiceLoginAuth") ) );
    req.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    nam.post(req, reqString);
}

void Authenticator::send2ndFactorPin(QString pin)
{
    auth_phase = 6;
    QString r = QString("timeStmp=" + timeStmp + "&secTok="+secTok);
    r+=QString("&smsUserPin="+pin);
    r+="&smsVerifyPin=Verify&smsToken=&checkedConnection=youtube:73:0&checkedDomains=youtube&PersistentCookie=on&PersistentOptionSelection=1&pstMsg=0&skipvpage=true";
    QByteArray reqString(r.toUtf8());
    QNetworkRequest req( QUrl( QString("https://accounts.google.com/SecondFactor") ) );
    req.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    nam.post(req, reqString);
}

QList<QNetworkCookie> Authenticator::getCookies()
{
    return sessionCookies;
}

bool Authenticator::amILoggedIn()
{
    int i = 0;
    foreach(QNetworkCookie cookie, sessionCookies) {
        if (cookie.name()=="APISID" || cookie.name()=="HSID" || cookie.name()=="S" || cookie.name()=="SAPISID" || cookie.name()=="SID" || cookie.name()=="SSID")
            i++;
    }
    if (i>=6)
        return true;
    return false;
}

void Authenticator::updateCookies(QList<QNetworkCookie> cookies)
{
    QJsonObject obj;
    sessionCookies.clear();
    QFile cookieFile(homePath);
   if (cookieFile.exists()) {
       cookieFile.open(QIODevice::ReadWrite | QIODevice::Text);
       cookieFile.resize(0);
       foreach(QNetworkCookie cookie, cookies) {
               qDebug() << "GOOD: " << cookie.name() << " - " << cookie.domain() << " - " << cookie.isSessionCookie() << " - " << cookie.expirationDate().toString();
               obj[cookie.name()] = QJsonValue( QString(cookie.value()) );
               sessionCookies.append(cookie);
       }
       QJsonDocument doc(obj);
       QFile cookieFile(homePath);
       if ( cookieFile.open(QIODevice::WriteOnly))
               {
           cookieFile.write(doc.toJson(), doc.toJson().size());
           }
       cookieFile.close();

         qDebug() << "Written cookies to " << homePath;
         auth_phase=5;

         emit(gotCookies());
     }
}

void Authenticator::auth()
{
    QFile cookieFile(homePath);
   if (cookieFile.exists()) {
       cookieFile.open(QIODevice::ReadOnly | QIODevice::Text);
       QString val = cookieFile.readAll();
       cookieFile.close();
       QJsonDocument doc = QJsonDocument::fromJson(val.toUtf8());
       QJsonObject obj = doc.object();
       foreach (QString s, obj.keys())
       {
           //TODO: check and delete
           if (s=="ACCOUNT_CHOOSER" || s=="GALX" || s=="GAPS" || s=="LSID" || s=="NID") continue;
           QNetworkCookie tmp;
           //qDebug() << s;
           tmp.setName(QVariant(s).toByteArray());
           tmp.setValue(obj.value(s).toVariant().toByteArray());
           tmp.setDomain(".google.com");

           sessionCookies.append(tmp);
       }
       emit(gotCookies());
        //qDebug() << "Already in!";

    }
    else {
        auth_phase = 0;
        QObject::connect(&nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(cb(QNetworkReply *)));
        nam.setCookieJar(&cJar);
        emit loginNeeded();
        getGalxToken();
    }
}

Authenticator::Authenticator()
{
}
