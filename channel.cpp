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

#include "messagefield.h"

static QString user_agent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/39.0.2171.71 Safari/537.36";
static qint32 MAX_READ_BYTES = 1024 * 1024;
static QString homePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/cookies.json";
static QString ORIGIN_URL = "https://talkgadget.google.com";

void Channel::processCookies(QNetworkReply *reply)
{
    bool cookieUpdated = false;
    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);

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
        nam->setCookieJar(new QNetworkCookieJar(this));
    }
}

void Channel::checkChannel()
{
    qDebug() << "Checking chnl";
    if (lastPushReceived.secsTo(QDateTime::currentDateTime()) > 30) {
        if (LPrep != NULL)
            LPrep->abort();

        qDebug() << "Dead, here I should sync al evts from last ts and notify QML that we're offline";
        qDebug() << "start new lpconn";
        if (isOnline && channelError == false)
            emit(channelLost());

        channelError = true;
        QTimer::singleShot(500, this, SLOT(longPollRequest()));
    }
}

void Channel::checkChannelAndReconnect()
{
    qDebug() << "Checking chnl";
    if (lastPushReceived.secsTo(QDateTime::currentDateTime()) > 30) {
        if (LPrep != NULL)
            LPrep->abort();
        qDebug() << "Dead, here I should sync al evts from last ts and notify QML that we're offline";
        qDebug() << "start new lpconn";
        if (isOnline && channelError == false)
            emit(channelLost());

        channelError = true;
    }
    QTimer::singleShot(500, this, SLOT(longPollRequest()));
}

void Channel::fastReconnect()
{
    qDebug() << "fast reconnecting";

    //I need to check whether the channel was lost as well; in case it was, first sync and then reconnect, otherwise simply reconnect
    //QTimer::singleShot(500, this, SLOT(longPollRequest()));
    checkChannelAndReconnect();
}

void Channel::setStatus(bool online) {
    isOnline = online;
}

Channel::Channel(QList<QNetworkCookie> cookies, QString ppath, QString pclid, QString pec, QString pprop, User pms, ConversationModel *cModel, RosterModel *rModel)
{
    nam = new QNetworkAccessManager();

    LPrep = NULL;
    channelError = false;
    fetchingSid = false;
    channelEstablishmentOccurring = false;
    lastIncomingConvId = "";
    lastValidParcelId = 4;
    isOnline = true;

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
    checkChannelTimer = new QTimer();
    QObject::connect(checkChannelTimer, SIGNAL(timeout()), this, SLOT(checkChannel()));
    checkChannelTimer->start(30000);
}

void Channel::nrf()
{
    channelEstablishmentOccurring = false;
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    processCookies(reply);
    qDebug() << reply->isReadable();
    qDebug() << "FINISHED called! " << channelError << "; " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QString srep = reply->readAll();
    qDebug() << srep;
    if (srep.contains("Unknown SID")) {
        //Need new SID, but new fetch should already be triggered by 400
        //fetchNewSid();
        return;
    }

    checkChannelTimer->start(30000);
    //If there's a network problem don't do anything, the connection will be retried by checkChannelStatus
    if (!channelError) {
            longPollRequest();
    }
}

void Channel::parseChannelData(QString sreply)
{
    int idx=0;
    //Replace quotes inside messages with html quotes
    sreply = sreply.replace("\\\\\\\\\\\\\\\"","&#34;");
    for (;;) {
        idx = sreply.indexOf('[', idx); // at the beginning there is a length, which we ignore.
        //qDebug() << idx;
        // ignore empty messages
        if (idx == -1) return;
        qDebug() << "##" << sreply.right(sreply.size()-idx);
        auto completeParsedReply = MessageField::parseListRef(sreply.leftRef(-1), idx);
        //qDebug() << idx;
        //qDebug() << completeParsedReply.size();
        if (!completeParsedReply.size())
            continue;
        //In very rare cases, there is really a list in the parcel: e.g. not size[[c1]]size[[c2]], but size[[c1],[c2]]
        //This case should be caught looping here
        for (int listidx = 0; listidx < completeParsedReply.size(); listidx++) {
            auto parsedReply = completeParsedReply[listidx].list();
            if (parsedReply.size()<2)
                continue;
            int parcelID = parsedReply[0].number().toInt();
            if (parcelID > lastValidParcelId + 1) {
                //we lost something, should check back
                qDebug() << "We lost " << parcelID - lastValidParcelId << " parcels";
                emit(channelRestored(lastValidParcelIdTS.addMSecs(50)));
            }
            lastValidParcelId = parcelID;
            lastValidParcelIdTS = QDateTime::currentDateTime();
            auto content = parsedReply[1].list();
            if (content.size() < 1) continue;
            auto cMap = content[0].map();

            qDebug() << cMap.size();
            if (cMap.size() < 2)
                continue;

            int idx3=0;
            QString dbgstr = cMap[1].string().replace("\\\"","\"").replace("\\\\\"","\\\"");
            QStringRef ref = dbgstr.leftRef(-1);
            auto innerMap = MessageField::parseListRef(ref, idx3);
            if (innerMap.size()<4)
                continue;
            innerMap = innerMap[3].map();
            //This is the innermost map, we want "2":[cbu]
            if (innerMap.size()<4)
                continue;

            QString i3 = innerMap[3].string();
            if (i3.startsWith("lcsw_hangouts")) {
                header_client = i3;
                emit updateClientId(header_client);
                addChannelService();
                return;
            }

            int idx2 = 0;
            QString cleanContent = i3.replace("\\\"","\"");
            auto parsedInner = MessageField::parseListRef(cleanContent.leftRef(-1), idx2);
            if (!parsedInner.size())
                continue;
            if (!parsedInner[0].string().contains("cbu")) continue;

            auto playloadList = parsedInner[1].list();
            if (!playloadList.size()) continue; // TODO is that senseful ??
            playloadList = playloadList[0].list();

            if (!playloadList.size() || !playloadList[0].list().size()) continue; // TODO is that senseful ??
            int as = playloadList[0].list()[0].number().toInt();
            qDebug() << as;
            emit activeClientUpdate(as);
            if (playloadList.size() < 2)
                continue;
            if (playloadList[2].type() == MessageField::List) {
                // parse events
                auto eventDataList = playloadList[2].list();

                for (auto eventData : eventDataList) {
                    Event evt = Utils::parseEvent(eventData.list());
                    qDebug() << "evt parsed";

                    //Eventually update the notificationLevel for this conversation
                    //rosterModel->updateNotificationLevel(evt.conversationId, evt.notificationLevel);

                    if (evt.isRenameEvent) {
                        qDebug() << "This is a rename event!";
                        emit renameConversation(evt.conversationId, evt.newName);
                    }

                    //I want to check whether I sent this message in order to keep the conversation view consistent
                    if (evt.sender.chat_id == myself.chat_id) {
                        evt.isMine = true;
                    }
                    if (evt.value.segments.size() == 0 && evt.value.attachments.size()==0) {
                        qDebug() << "No segs! Skipping";
                        continue;
                    }

                    bool added = conversationModel->addEventToConversation(evt.conversationId, evt);
                    rosterModel->putOnTop(evt.conversationId);
                    qDebug() << "Added";
                    lastIncomingConvId = evt.conversationId;
                    if (added && evt.sender.chat_id != myself.chat_id) {
                        qDebug() << "Going to notify";
                        //Signal new event only if the actual conversation isn't already visible to the user
                        if (appPaused || (conversationModel->getCid() != evt.conversationId)) {
                            rosterModel->addUnreadMsg(evt.conversationId);
                            //If notificationLevel == 10 the conversation has been silenced -> don't notify
                            if (evt.notificationLevel!=QUIET) {
                                if (evt.value.segments.size()==0)
                                    emit showNotification("", evt.sender.chat_id, "", evt.sender.chat_id,1,evt.conversationId);
                                else
                                    emit showNotification(evt.value.segments[0].value, evt.sender.chat_id, evt.value.segments[0].value, evt.sender.chat_id,1,evt.conversationId);
                            }
                        }
                        else {
                            //Update watermark, since I've read the message; if notification level this should be the active client
                            emit updateWM(evt.conversationId);
                        }
                    }
                }
            }
            //set focus notification
            // playloadList[3]

            if (playloadList.size() < 5) continue; // TODO should we really continue?
            // typing notification
            qDebug() << playloadList[4].type();
            if (playloadList[4].type() == MessageField::List) {
                auto typingData = playloadList[4].list();
                ChannelEvent evt;
                // parseTypingNotification():
                for (int i = 0; i < typingData.size(); ++i) {
                    if (typingData[i].type() == MessageField::List) {
                        auto currentList = typingData[i].list();
                        if (i == 0)
                            evt.conversationId = currentList[0].string();
                        else if (i == 1)
                            // here currentList contains the Identity, so extract the chat_id:
                            evt.userId = currentList[0].string();
                    } else if (typingData[i].type() == MessageField::Number) {
                        if (i == 3)
                            evt.typingStatus = typingData[i].number().toInt();
                    }
                }
                qDebug() << "Typing" << evt.userId << "in" << evt.conversationId << "state" << evt.typingStatus;

                if (!evt.userId.contains(myself.chat_id))
                    emit isTyping(evt.conversationId, evt.userId, evt.typingStatus);
            }

            if (playloadList.size() < 6) continue; // TODO should we really continue?
            //notification level; wasn't this already in the event info?
            // playloadList[5]
            if (playloadList[5].type() == MessageField::List) {
                auto notifLevelList = playloadList[5].list();
                if (notifLevelList.size() == 4) {
                    NotifLevelUpdate evt;
                    evt.convId = notifLevelList[0].list()[0].string();
                    evt.newState = (NotificationLevel)notifLevelList[1].number().toInt();
                    //NotificationLevel update
                    rosterModel->updateNotificationLevel(evt.convId, evt.newState);
                }
            }

            // playloadList[6] would be reply to invite
            if (playloadList.size() < 8) continue; // TODO should we really continue?
            // parseReadStateNotification():
            if (playloadList[7].type() == MessageField::List) {
                ReadState evt;
                auto readStateInfo = playloadList[7].list();
                for (int i = 0; i < readStateInfo.size(); ++i) {
                    if (readStateInfo[i].type() == MessageField::List) {
                        auto currentList = readStateInfo[i].list();
                        if (i == 0)
                            evt.userid = Utils::parseIdentity(currentList);
                        else if (i == 1)
                            evt.convId = currentList[0].string();
                    } else if (readStateInfo[i].type() == MessageField::Number) {
                        if (i == 2)
                            evt.last_read = QDateTime::fromMSecsSinceEpoch(readStateInfo[i].number().toLongLong() / 1000);
                    }
                }
                conversationModel->updateReadState(evt);
                qDebug() << "Done parsing";
            }
        }
    }
}

void Channel::nr()
{
    channelEstablishmentOccurring = false;
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    processCookies(reply);

    QString sreply = reply->read(MAX_READ_BYTES);
    qDebug() << sreply;
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
        qDebug() << "Gonna emit channelRestored";
        emit(channelRestored(lastPushReceived.addMSecs(50)));
        channelError = false;
    }
    else {
    }
    lastPushReceived = QDateTime::currentDateTime();

    checkChannelTimer->start(30000);

    parseChannelData(sreply);
    //if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()!=200)
      //  longPollRequest();
}

void Channel::addChannelService() {
    //Add service to channel; copy-paste from chrome
    QNetworkRequest req(QUrl(QString("https://0.client-channel.google.com/client-channel/channel/bind?ctype=hangouts&VER=8&RID=81188&gsessionid=" + gsessionid + "&SID=" + sid)));
    //{"3": {"1": {"1": "babel"}}}))
    QVariant body = "count=1&ofs=1&req0_p=%7B%221%22%3A%7B%221%22%3A%7B%221%22%3A%7B%221%22%3A3%2C%222%22%3A2%7D%7D%2C%222%22%3A%7B%221%22%3A%7B%221%22%3A3%2C%222%22%3A2%7D%2C%222%22%3A%22%22%2C%223%22%3A%22JS%22%2C%224%22%3A%22lcsclient%22%7D%2C%223%22%3A1446490244140%2C%224%22%3A1446490244118%2C%225%22%3A%22c3%22%7D%2C%223%22%3A%7B%221%22%3A%7B%221%22%3A%22babel%22%7D%7D%7D";
    QList<QNetworkCookie> reqCookies;
    foreach (QNetworkCookie cookie, session_cookies) {
            reqCookies.append(cookie);
    }
    req.setRawHeader("authorization", getAuthHeader());
    req.setRawHeader("x-origin", QVariant::fromValue(ORIGIN_URL).toByteArray());
    req.setRawHeader("x-goog-authuser", "0");
    req.setRawHeader("content-type", "application/x-www-form-urlencoded;charset=utf-8");
    req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(reqCookies));
    nam->post(req, body.toByteArray());
}

void Channel::parseSid()
{
    fetchingSid = false;
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    processCookies(reply);

    if (reply->error() == QNetworkReply::NoError) {
        QString rep = reply->readAll();
        qDebug() << "SID##" << rep;

        int start = rep.indexOf("[");

        auto parsedData = MessageField::parseListRef(rep.leftRef(-1), start);
        //ROW 0
        if (parsedData.size() < 1 || parsedData[0].list().size() < 2 || parsedData[0].list()[1].list().size() < 2)
            return;
        sid = parsedData[0].list()[1].list()[1].string();
        gsessionid = parsedData[1].list()[1].list()[0].map()[1].string();
    }
    else {
        QString rep = reply->readAll();
        qDebug() << rep;
        qDebug() << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }
    reply->deleteLater();

    checkChannelTimer->start(30000);
    //Now the reply should be std
    longPollRequest();
}

void Channel::slotError(QNetworkReply::NetworkError err)
{
    fetchingSid = false;
    channelEstablishmentOccurring = false;
    channelError = true;
    qDebug() << "Error, retrying to activate channel: " << err;

    //I have error 8 after long inactivity, and the connection can't be reestablished, let's try the following
    //OperationCanceledError is instead triggered by the LPRequst, but if this happens it means that there's an operation stuck for more than 30 seconds; let's try to create a new QNetworkAccessManager and see if it helps
    if (err == QNetworkReply::OperationCanceledError || err==QNetworkReply::NetworkSessionFailedError) {
        nam->deleteLater();
        LPrep = NULL;
        nam = new QNetworkAccessManager();
        //emit qnamUpdated(nam);
        //QNetworkSession session( nam->configuration() );
        //emit cancelAllActiveRequests();
        //session.open();

        nam->setCookieJar(new QNetworkCookieJar(this));
        emit cookieUpdateNeeded(QNetworkCookie());
    }
    checkChannelTimer->start();
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
    //If LPrep != null I may have a timeout for QTNetworkManagerAccess, will be handled in slotError
    if (LPrep != NULL) {
            LPrep->abort();
            LPrep->deleteLater();
            LPrep = NULL;
        }
    if (channelEstablishmentOccurring) {
        qDebug() << "Another req is already active, returning";
        return;
    }
    if (!isOnline) {
        qDebug() << "Not online";
        return;
    }
    channelEstablishmentOccurring = true;

    qDebug() << sid;
    qDebug() << gsessionid;

    QString body = "?VER=8&RID=rpc&ctype=hangouts&t=1&CI=0&gsessionid=" + gsessionid + "&SID=" + sid;
    QNetworkRequest req(QUrl(QString("https://0.client-channel.google.com/client-channel/channel/bind" + body)));
    req.setRawHeader("User-Agent", QVariant::fromValue(user_agent).toByteArray());
    req.setRawHeader("Connection", "Keep-Alive");
    req.setRawHeader("authorization", getAuthHeader());
    req.setRawHeader("x-origin", QVariant::fromValue(ORIGIN_URL).toByteArray());
    req.setRawHeader("x-goog-authuser", "0");


    req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(session_cookies));
    qDebug() << "Making lp req";
    LPrep = nam->get(req);
    QObject::connect(LPrep, SIGNAL(readyRead()), this, SLOT(nr()));
    QObject::connect(LPrep, SIGNAL(finished()), this, SLOT(nrf()));
    QObject::connect(LPrep, SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(slotError(QNetworkReply::NetworkError)));
    //QObject::connect(this, SIGNAL(cancelAllActiveRequests()), LPrep, SLOT(abort()));
}

QByteArray Channel::getAuthHeader()
{
    QByteArray res = "SAPISIDHASH ";
    qint64 time_msec = QDateTime::currentMSecsSinceEpoch();//1000;
    QString auth_string = QString::number(time_msec);
    auth_string += " ";
    foreach (QNetworkCookie cookie, session_cookies) {
        if (cookie.name()=="SAPISID")
            auth_string += cookie.value();
    }
    auth_string += " ";
    auth_string += ORIGIN_URL;
    res += QString::number(time_msec);
    res += "_";
    res += QCryptographicHash::hash(auth_string.toUtf8(), QCryptographicHash::Sha1).toHex();
    return res;
}

void Channel::fetchNewSid()
{
    if (fetchingSid)
        return;
    fetchingSid = true;

    qDebug() << "fetch new sid";
    QNetworkRequest req(QUrl(QString("https://0.client-channel.google.com/client-channel/channel/bind")));
    QVariant body = "ctype=hangouts&VER=8&RID=81188";
    QList<QNetworkCookie> reqCookies;
    foreach (QNetworkCookie cookie, session_cookies) {
            reqCookies.append(cookie);
    }
    req.setRawHeader("authorization", getAuthHeader());
    req.setRawHeader("x-origin", QVariant::fromValue(ORIGIN_URL).toByteArray());
    req.setRawHeader("x-goog-authuser", "0");
    req.setRawHeader("content-type", "application/x-www-form-urlencoded;charset=utf-8");
    req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(reqCookies));
    QNetworkReply *rep = nam->post(req, body.toByteArray());
    QObject::connect(rep, SIGNAL(finished()), this, SLOT(parseSid()));
    //QObject::connect(this, SIGNAL(cancelAllActiveRequests()), rep, SLOT(abort()));
}

void Channel::listen()
{
    bool need_new_sid = true;

    if (need_new_sid) {
        fetchNewSid();
        need_new_sid = false;
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
