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

void Channel::checkChannel()
{
    qDebug() << "Checking chnl";
    if (lastPushReceived.secsTo(QDateTime::currentDateTime()) > 30) {
        qDebug() << "Dead, here I should sync al evts from last ts and notify QML that we're offline";
        qDebug() << "start new lpconn";
        channelError = true;
        emit(channelLost());
        QTimer::singleShot(500, this, SLOT(longPollRequest()));
    }
}

void Channel::checkChannelAndReconnect()
{
    qDebug() << "Checking chnl";
    if (lastPushReceived.secsTo(QDateTime::currentDateTime()) > 30) {
        qDebug() << "Dead, here I should sync al evts from last ts and notify QML that we're offline";
        qDebug() << "start new lpconn";
        channelError = true;
        emit(channelLost());
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
        //Need new SID, but new fetch should already be triggered by 400
        //fetchNewSid();
        return;
    }
    //If there's a network problem don't do anything, the connection will be retried by checkChannelStatus
    if (!channelError) longPollRequest();
}

void Channel::parseChannelData(QString sreply)
{
    int idx=0;
    for (;;) {
        idx = sreply.indexOf('[', idx); // at the beginning there is a length, which we ignore.
        qDebug() << idx;
        // ignore empty messages
        if (idx == -1) return;
        qDebug() << "##" << sreply.right(sreply.size()-idx);
        auto completeParsedReply = MessageField::parseListRef(sreply.leftRef(-1), idx);
        qDebug() << idx;
        qDebug() << completeParsedReply.size();
        if (!completeParsedReply.size())
            continue;
        //In very rare cases, there is really a list in the parcel: e.g. not size[[c1]]size[[c2]], but size[[c1],[c2]]
        //This case should be caought looping here
        for (int listidx = 0; listidx < completeParsedReply.size(); listidx++) {
            auto parsedReply = completeParsedReply[listidx].list();
            qDebug() << parsedReply.size();
            if (parsedReply.size()<2)
                continue;
            auto content = parsedReply[1].list();
            //qDebug() << content[0].string();
            if (content.size() < 2) continue;
            if (!content[0].string().contains("c")) continue;
            auto cContent = content[1].list();
            if (cContent.size() < 2) continue;
            cContent = cContent[1].list();
            // TODO could there be multiple bfo ??
        //    qDebug() << cContent[0].string().contains("bfo");

            if (cContent.size() < 1 || !cContent[0].string().contains("bfo")) {
                continue;
                /*
                 * sreply.remove(0, idx+1);
                if (sreply.isEmpty()) continue;
                qDebug() << "Removing and reparsing";
                parseChannelData(sreply);
                continue;
                */
            }

            // prepare the inner data to parse:
            qDebug() << cContent.size();
            //for (int a=0; a<cContent.size(); a++)
              //  qDebug() << cContent[a].string();

            auto stringData = cContent[1].string();
            stringData = Utils::cleanText(stringData);
            qDebug() << "inner data##" << stringData;
            // we can now parse the inner data:
            int idx2 = 0;
            auto parsedInner = MessageField::parseListRef(stringData.leftRef(-1), idx2);
            qDebug() << idx2;
            if (parsedInner.size()<2)
                continue;
            // TODO can there be multiple cbu in one reply?
            if (!parsedInner[0].string().contains("cbu")) continue;

            auto playloadList = parsedInner[1].list();
            if (!playloadList.size()) continue; // TODO is that senseful ??
            playloadList = playloadList[0].list();
            if (!playloadList.size() || !playloadList[0].list().size()) continue; // TODO is that senseful ??
            qDebug() << playloadList.size();
            int as = playloadList[0].list()[0].number().toInt();
            emit activeClientUpdate(as);
            if (playloadList.size() < 2)
                continue;
            if (playloadList[2].type() == MessageField::List) {
                // parse events
                auto eventDataList = playloadList[2].list();
                qDebug() << eventDataList.size();

                for (auto eventData : eventDataList) {
                    Event evt = Utils::parseEvent(eventData.list());
                    qDebug() << "evt parsed";
                    //This is old, on the channel? Skip
                    if (!evt.isOld) {
                        //I want to check whether I sent this message in order to keep the conversation view consistent
                        if (evt.sender.chat_id == myself.chat_id) {
                            evt.isMine = true;
                        }
                        if (evt.value.segments.size() == 0 && evt.value.attachments.size()==0) {
                            qDebug() << "No segs! Skipping";
                            continue;
                        }

                        conversationModel->addEventToConversation(evt.conversationId, evt);
                        qDebug() << "Added";
                        lastIncomingConvId = evt.conversationId;
                        if (evt.sender.chat_id != myself.chat_id) {
                            qDebug() << "Going to notify";
                            //Signal new event only if the actual conversation isn't already visible to the user
                            qDebug() << conversationModel->getCid();
                            qDebug() << evt.conversationId;
                            qDebug() << evt.notificationLevel;
                            if (appPaused || (conversationModel->getCid() != evt.conversationId)) {
                                rosterModel->addUnreadMsg(evt.conversationId);
                                //If notificationLevel == 10 the conversation has been silenced -> don't notify
                                if (evt.notificationLevel!=QUIET)
                                    if (evt.value.segments.size()==0)
                                        emit showNotification("", evt.sender.chat_id, "", evt.sender.chat_id,1,evt.conversationId);
                                    else
                                        emit showNotification(evt.value.segments[0].value, evt.sender.chat_id, evt.value.segments[0].value, evt.sender.chat_id,1,evt.conversationId);
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
            }
            //set focus notification
            // playloadList[3]

            if (playloadList.size() < 5) continue; // TODO should we really continue?
            // typing notification
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
        qDebug() << "Gonna emit channelRestored";
        emit(channelRestored(lastPushReceived.addMSecs(500)));
        channelError = false;
    }
    else {
        qDebug() << "Channel had no error";
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
        qDebug() << "SID##" << rep;

        int start = rep.indexOf("[");

        auto parsedData = MessageField::parseListRef(rep.leftRef(-1), start);
        //ROW 0
        if (parsedData.size() < 1 || parsedData[0].list().size() < 2 || parsedData[0].list()[1].list().size() < 2)
            return;
        sid = parsedData[0].list()[1].list()[1].string();
        qDebug() << sid;

        //ROW 1 and 2 discarded

        //ROW 3
        //TODO: add check for ALL these lists
        if (parsedData.size() < 4 || parsedData[3].list().size() < 2 || parsedData[3].list()[1].list().size() < 2 || parsedData[3].list()[1].list()[1].list().size() < 2)
            return;
        auto rowData = parsedData[3].list()[1].list()[1].list()[1].list();
        if (rowData[0].string()!="cfj") {
            //Not the right line! Try with the second one
            if (parsedData.size() < 4 || parsedData[2].list().size() < 2 || parsedData[2].list()[1].list().size() < 2 || parsedData[2].list()[1].list()[1].list().size() < 2)
                return;
            rowData = parsedData[2].list()[1].list()[1].list()[1].list();
            if (rowData[0].string()!="cfj") {
                qDebug() << "Couldn't find cfj line, returning";
                return;
            }
        }

        QString stringData = rowData[1].string();
        QStringList temp = stringData.split("/");
        if (temp.size() < 2) {
            qDebug() << "temp size is " << temp.size();
            qDebug() << stringData;
        }
        qDebug() << temp.at(0);
        email = temp.at(0);

        if (!email.contains(QChar('@')))
            //Something went wrong, may happen after channel reestablished
            //TODO: Try to go on with the old header_client, need some tests
            return;

        qDebug() << temp.at(1);
        header_client = temp.at(1);
        emit updateClientId(header_client);

        if (parsedData.size() < 5 || parsedData[4].list().size() < 2 || parsedData[4].list()[1].list().size() < 2 || parsedData[4].list()[1].list()[1].list().size() < 2)
            return;
        auto row4Data = parsedData[4].list()[1].list()[1].list()[1].list();
        gsessionid = row4Data[1].string();
        qDebug() << gsessionid;

    }
    else {
        QString rep = reply->readAll();
        qDebug() << rep;
        qDebug() << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }
    //Now the reply should be std
    longPollRequest();
}

void Channel::slotError(QNetworkReply::NetworkError err)
{
    channelError = true;
    qDebug() << err;
    qDebug() << "Error, retrying to activate channel";

    //I have error 8 after long inactivity, and the connection can't be reestablished, let's try the following
    if (err==8) {
        nam = new QNetworkAccessManager();
        emit qnamUpdated(nam);
    }

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
    QVariant body = "VER=8&RID=81187&clid=" + clid + "&prop=" + prop + "&ec="+ec;
    ////qDebug() << body.toString();
    QList<QNetworkCookie> reqCookies;
    foreach (QNetworkCookie cookie, session_cookies) {
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
