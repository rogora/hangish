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


#include "channel.h"

static QString user_agent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/39.0.2171.71 Safari/537.36";
static qint32 MAX_READ_BYTES = 1024 * 1024;
static QString homePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/cookies.json";

void Channel::checkChannel()
{
    qDebug() << "Checking chnl";
    if (lastPushReceived.secsTo(QDateTime::currentDateTime()) > 30) {
        qDebug() << "Dead, here I should sync al evts from last ts and notify QML that we're offline";
        qDebug() << "start new lpconn";
        channelError = true;
        emit(channelLost());
        /*
        if (LPrep != NULL) {
            LPrep->close();
            delete LPrep;
        }
        */
        QTimer::singleShot(500, this, SLOT(longPollRequest()));
    }
}

void Channel::fastReconnect()
{
    qDebug() << "fast reconnecting";
    /*
    if (LPrep != NULL) {
        LPrep->close();
        delete LPrep;
    }
    */
    QTimer::singleShot(500, this, SLOT(longPollRequest()));
}

Channel::Channel(QNetworkAccessManager *n, QList<QNetworkCookie> cookies, QString ppath, QString pclid, QString pec, QString pprop, User pms, ConversationModel *cModel, RosterModel *rModel)
{
    LPrep = NULL;
    channelError = false;
    lastIncomingConvId = "";

    nam = n;
    myself = pms;
    conversationModel = cModel;
    rosterModel = rModel;

    session_cookies = cookies;
    path = ppath;
    clid = pclid;
    ec = pec;
    prop = pprop;

    //Init
    lastPushReceived = QDateTime::currentDateTime();

    QTimer *timer = new QTimer(this);
    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(checkChannel()));
    timer->start(30000);
}

ChannelEvent Channel::parseTypingNotification(QString input, ChannelEvent evt)
{
    int start = 1;
    QString conversationId = Utils::getNextAtomicField(input, start);
    conversationId = conversationId.mid(1, conversationId.size()-2);
    if (conversationId=="") return evt;
    QString userId = Utils::getNextAtomicField(input, start);
    if (userId=="") return evt;
    QString ts = Utils::getNextAtomicField(input, start);
    if (ts=="") return evt;
    //QString typingStatus = Utils::getNextAtomicField(input, start);
    //No need to parse a field, I know what to expect
    QString typingStatus = input.mid(start, 1);
    evt.conversationId = conversationId.mid(1, conversationId.size()-2);
    evt.userId = userId;
    evt.typingStatus = typingStatus.toInt();

    qDebug() << "User " << userId << " in conv " << conversationId << " is typing " << typingStatus << " at " << ts;
    return evt;
}

void Channel::nrf()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    //Eventually update cookies, may set S
    bool cookieUpdated = false;
    foreach(QNetworkCookie cookie, c) {
        qDebug() << cookie.name();
        for (int i=0; i<session_cookies.size(); i++) {
            if (session_cookies[i].name() == cookie.name()) {
                session_cookies[i].setValue(cookie.value());
                emit cookieUpdateNeeded(cookie);
                qDebug() << "Updated cookie " << cookie.name();
                cookieUpdated = true;
            }
        }
    }
    if (cookieUpdated) {
        nam = new QNetworkAccessManager();
        emit qnamUpdated(nam);
    }
    qDebug() << "FINISHED called! " << channelError;
    QString srep = reply->readAll();
    qDebug() << srep;
    if (srep.contains("Unknown SID")) {
        //Need new SID
        fetchNewSid();
        return;
    }
    //If there's a network problem don't do anything, the connection will be retried by checkChannelStatus
    if (!channelError) longPollRequest();
}

void Channel::parseChannelData(QString sreply)
{
    qDebug() << sreply;
    sreply = sreply.remove("\\n");
    sreply = sreply.remove("\n");
    sreply = sreply.remove(QChar('\\'));
    QString parcel;
    int parcelCursor = 0;
    for (;;) {
        //We skip the int representing the (Javascript) len of the parcel
        parcelCursor = sreply.indexOf("[", parcelCursor);
        if (parcelCursor==-1)
            return;
        parcel = Utils::getNextAtomicFieldForPush(sreply, parcelCursor);
        ////qDebug() << "PARCEL: " << parcel;
        if (parcel.size() < 20)
            return;

        int start = parcel.indexOf("\"c\"");
        if (start==-1)
            return;
        QString payload = Utils::getNextField(parcel, start + 4);
        //skip id
        start = 1;
        Utils::getNextAtomicField(payload, start);
        //and now the actual payload
        payload = Utils::getNextAtomicField(payload, start);
        QString type;
        start = 1;
        type = Utils::getNextAtomicField(payload, start);
        ////qDebug() << type;
        if (type != "\"bfo\"")
            continue;
        //and now the actual payload
        payload = Utils::getNextAtomicField(payload, start);
        //////qDebug() << "pld1: " << payload;

        start = 2;
        type = Utils::getNextAtomicField(payload, start);
        ////qDebug() << type;
        if (type != "\"cbu\"")
            continue;

        //This, finally, is the real payload
        int globalptr = 1;
        QString arr_payload = Utils::getNextField(payload, start);
        //////qDebug() << "arr pld: " << arr_payload;
        //But there could be more than 1
        for (;;) {

            payload = Utils::getNextAtomicField(arr_payload, globalptr);
            ////qDebug() << "pld: " << payload;
            if (payload.size() < 30) break;
            ChannelEvent cevt;
            start = 1;
            //Info about active clients -- if I'm not the active client then I must disable notifications!
            QString header = Utils::getNextAtomicField(payload, start);
            qDebug() << header;
            if (header.size()>10)
                {
                    QString newId;
                    int as = Utils::parseActiveClientUpdate(header, newId);
                    emit activeClientUpdate(as);
                }
            //conv notification; always none?
            Utils::getNextAtomicField(payload, start);
            //evt notification -- this holds the actual message
            QString evtstring = Utils::getNextAtomicFieldForPush(payload, start);
            qDebug() << evtstring;
            if (evtstring.size() > 10) {
            //evtstring has [evt, None], so we catch only the first value
                Event evt = Utils::parseEvent(Utils::getNextField(evtstring, 1));
                if (evt.value.valid) {
                    qDebug() << evt.sender.chat_id << " sent " << evt.value.segments[0].value;
                    conversationModel->addEventToConversation(evt.conversationId, evt);
                    lastIncomingConvId = evt.conversationId;
                    if (evt.sender.chat_id != myself.chat_id) {
                        //Signal new event only if the actual conversation isn't already visible to the user
                        qDebug() << conversationModel->getCid();
                        qDebug() << evt.conversationId;
                        qDebug() << evt.notificationLevel;
                        if (appPaused || (conversationModel->getCid() != evt.conversationId)) {
                            rosterModel->addUnreadMsg(evt.conversationId);
                            //If notificationLevel == 10 the conversation has been silenced -> don't notify
                            if (evt.notificationLevel==RING)
                                emit showNotification(evt.value.segments[0].value, evt.sender.chat_id, evt.value.segments[0].value, evt.sender.chat_id);
                        }
                        else {
                            //Update watermark, since I've read the message; if notification level this should be the active client
                            emit updateWM(evt.conversationId);
                        }
                    }
                }
                else {
                    qDebug() << "Invalid evt received";
                }
            }
            //set focus notification
            Utils::getNextAtomicField(payload, start);
            //set typing notification
            QString typing = Utils::getNextAtomicFieldForPush(payload, start);
            qDebug() << "typing string " << typing;
            if (typing.size() > 3) {
                cevt = parseTypingNotification(typing, cevt);
                //I would know wether myself is typing :)
                if (!cevt.userId.contains(myself.chat_id))
                    emit isTyping(cevt.conversationId, cevt.userId, cevt.typingStatus);
            }
            //notification level; wasn't this already in the event info?
            QString notifLev = Utils::getNextAtomicField(payload, start);
            qDebug() << notifLev;
            //reply to invite
            Utils::getNextAtomicField(payload, start);
            //watermark
            QString wmNotification = Utils::getNextAtomicField(payload, start);
            qDebug() << wmNotification;
            if (wmNotification.size()>10) {
                conversationModel->updateReadState(Utils::parseReadStateNotification(wmNotification));
            }
            //None?
            Utils::getNextAtomicField(payload, start);
            //settings
            Utils::getNextAtomicField(payload, start);
            //view modification
            Utils::getNextAtomicField(payload, start);
            //easter egg
            Utils::getNextAtomicField(payload, start);
            //conversation
            QString conversation = Utils::getNextAtomicField(payload, start);
            ////qDebug() << "CONV: " << conversation;

            //self presence
            Utils::getNextAtomicField(payload, start);
            //delete notification
            Utils::getNextAtomicField(payload, start);
            //presence notification
            Utils::getNextAtomicField(payload, start);
            //block notification
            Utils::getNextAtomicField(payload, start);
            //invitation notification
            Utils::getNextAtomicField(payload, start);
        }
    }
}

void Channel::nr()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    //Eventually update cookies, may set S
    bool cookieUpdated = false;
    foreach(QNetworkCookie cookie, c) {
        qDebug() << cookie.name();
        for (int i=0; i<session_cookies.size(); i++) {
            if (session_cookies[i].name() == cookie.name()) {
                session_cookies[i].setValue(cookie.value());
                emit cookieUpdateNeeded(cookie);
                qDebug() << "Updated cookie " << cookie.name();
                cookieUpdated = true;
            }
        }
    }
    if (cookieUpdated) {
        nam = new QNetworkAccessManager();
        emit qnamUpdated(nam);
    }

    QString sreply = reply->read(MAX_READ_BYTES);
    ////qDebug() << "Got reply for lp " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    ///
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==401) {
        qDebug() << "Auth expired!";
    }
    else if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==400) {
        //Need new SID?
        qDebug() << sreply;
        qDebug() << "New seed needed?";
        fetchNewSid();
        qDebug() << "fetched";
        return;
    }


    if (lastIncompleteParcel!="")
        sreply = QString(lastIncompleteParcel + sreply);
    if (!sreply.endsWith("]\n"))
    {
        qDebug() << "Incomplete parcel";
        lastIncompleteParcel = sreply;
        return;
    }
    else lastIncompleteParcel = "";

    //if I'm here it means the channel is working fine
    if (channelError) {
        emit(channelRestored(lastPushReceived.addMSecs(500)));
        channelError = false;
    }
    lastPushReceived = QDateTime::currentDateTime();

    parseChannelData(sreply);
    //if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()!=200)
      //  longPollRequest();
}

void Channel::parseSid()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    //Eventually update cookies, may set S
    bool cookieUpdated = false;
    foreach(QNetworkCookie cookie, c) {
        qDebug() << cookie.name();
        for (int i=0; i<session_cookies.size(); i++) {
            if (session_cookies[i].name() == cookie.name()) {
                session_cookies[i].setValue(cookie.value());
                emit cookieUpdateNeeded(cookie);
                qDebug() << "Updated cookie " << cookie.name();
                cookieUpdated = true;
            }
        }
    }
    if (cookieUpdated) {
        nam = new QNetworkAccessManager();
        emit qnamUpdated(nam);
    }
    if (reply->error() == QNetworkReply::NoError) {
        QString rep = reply->readAll();
        qDebug() << rep;
        int start, start2=1, tmp=1;
        //ROW 0
        start = rep.indexOf("[");
        QString zero = Utils::getNextAtomicField(Utils::getNextAtomicField(rep, start), start2);
        qDebug() << zero;
        //Skip 1
        Utils::getNextAtomicField(zero, tmp);
        zero = Utils::getNextAtomicField(zero, tmp);
        tmp = 1;
        //Skip 1
        Utils::getNextAtomicField(zero, tmp);
        sid = Utils::getNextAtomicField(zero, tmp);
        sid = sid.mid(1, sid.size()-2);
        qDebug() << sid;

        //ROW 1 and 2 discarded
        tmp = 1;
        start2 = rep.indexOf("[", start2);
        QString one = Utils::getNextAtomicField(rep, start2);
        qDebug() << one;
        start2 = rep.indexOf("[", start2);
        QString two = Utils::getNextAtomicField(rep, start2);
        qDebug() << two;


        //ROW 3
        start2 = rep.indexOf("[", start2);
        QString three = Utils::getNextAtomicField(rep, start2);
        qDebug() << three;
        //Skip 1
        Utils::getNextAtomicField(three, tmp);
        three = Utils::getNextAtomicField(three, tmp);
        tmp = 1;
        //Skip 1
        Utils::getNextAtomicField(three, tmp);
        three = Utils::getNextAtomicField(three, tmp);
        tmp = 1;
        //Skip 1
        Utils::getNextAtomicField(three, tmp);
        three = Utils::getNextAtomicField(three, tmp);
        tmp = 1;

        Utils::getNextAtomicField(three, tmp);
        email = Utils::getNextAtomicField(three, tmp);
        email = email.mid(1, email.size()-2);
        QStringList temp = email.split("/");
        qDebug() << temp.at(0);
        email = temp.at(0);

        if (!email.contains(QChar('@')))
            //Something went wrong, may happen after channel reestablished
            //TODO: Try to go on with the old header_client, need some tests
            return;

        qDebug() << temp.at(1);
        header_client = temp.at(1);
        emit updateClientId(header_client);

        //ROW 4
        start2 = rep.indexOf("[", start2);
        QString four = Utils::getNextAtomicField(rep, start2);
        qDebug() << four;
        //Skip 1
        Utils::getNextAtomicField(four, tmp);
        four = Utils::getNextAtomicField(four, tmp);
        tmp = 1;
        //Skip 1
        Utils::getNextAtomicField(four, tmp);
        four = Utils::getNextAtomicField(four, tmp);
        tmp = 1;
        //Skip 1
        Utils::getNextAtomicField(four, tmp);
        four = Utils::getNextAtomicField(four, tmp);
        tmp = 1;

        Utils::getNextAtomicField(four, tmp);
        gsessionid = Utils::getNextAtomicField(four, tmp);
        gsessionid = gsessionid.mid(1, gsessionid.size()-2);

    }
    else {
        QString rep = reply->readAll();
        qDebug() << rep;
        qDebug() << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }
    //Now the reply should be std
    if (!channelError) longPollRequest();
}

void Channel::slotError(QNetworkReply::NetworkError err)
{
    channelError = true;
    qDebug() << err;
    qDebug() << "Error, retrying to activate channel";
    //longPollRequest();

    /*
    //I may have to retry establishing a connection here, if timers (and only them) are suspended on deep-sleep
    //5 means canceled by user
    if (err != QNetworkReply::OperationCanceledError && err!=QNetworkReply::NetworkSessionFailedError)
        longPollRequest();

    //This may happen temporarily during reconnection
    if (err == QNetworkReply::NetworkSessionFailedError)
        QTimer::singleShot(5000, this, SLOT(LPRSlot()));
        */
}

void Channel::longPollRequest()
{
    if (LPrep != NULL) {
        LPrep->close();
        delete LPrep;
        LPrep = NULL;
    }
    //for (;;) {
    QString body = "?VER=8&RID=rpc&t=1&CI=0&clid=" + clid + "&prop=" + prop + "&gsessionid=" + gsessionid + "&SID=" + sid + "&ec="+ec;
    QNetworkRequest req(QUrl(QString("https://talkgadget.google.com" + path + "bind" + body)));
    req.setRawHeader("User-Agent", QVariant::fromValue(user_agent).toByteArray());
    req.setRawHeader("Connection", "Keep-Alive");

    req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(session_cookies));
    qDebug() << "Making lp req";
    LPrep = nam->get(req);
    QObject::connect(LPrep, SIGNAL(readyRead()), this, SLOT(nr()));
    QObject::connect(LPrep, SIGNAL(finished()), this, SLOT(nrf()));
    QObject::connect(LPrep,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(slotError(QNetworkReply::NetworkError)));
   // }
}

void Channel::fetchNewSid()
{
    qDebug() << "fetch new sid";
    QNetworkRequest req(QString("https://talkgadget.google.com" + path + "bind"));
    ////qDebug() << req.url().toString();
    //QVariant body = "{\"VER\":8, \"RID\": 81187, \"clid\": \"" + clid + "\", \"ec\": \"" + ec + "\", \"prop\": \"" + prop + "\"}";
    QVariant body = "VER=8&RID=81187&clid=" + clid + "&prop=" + prop + "&ec="+ec;
    ////qDebug() << body.toString();
    QList<QNetworkCookie> reqCookies;
    foreach (QNetworkCookie cookie, session_cookies) {
        //if (cookie.name()=="SAPISID" || cookie.name()=="SAPISID" || cookie.name()=="HSID" || cookie.name()=="APISID" || cookie.name()=="SID")
            reqCookies.append(cookie);
    }
    req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(reqCookies));
    QNetworkReply *rep = nam->post(req, body.toByteArray());
    QObject::connect(rep, SIGNAL(finished()), this, SLOT(parseSid()));
    ////qDebug() << "posted";
}

void Channel::listen()
{
    static int MAX_RETRIES = 1;
    int retries = MAX_RETRIES;
    bool need_new_sid = true;

    while (retries > 0) {
        if (need_new_sid) {
            fetchNewSid();
            need_new_sid = false;
        }
        retries--;
    }
}

QDateTime Channel::getLastPushTs()
{
    return lastPushReceived;
}

void Channel::setAppOpened()
{
    appPaused = false;
}

void Channel::setAppPaused()
{
    appPaused = true;
}

QString Channel::getLastIncomingConversation() {
    return lastIncomingConvId;
}
