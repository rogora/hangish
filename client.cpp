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


#include "client.h"
#include "messagefield.h"

static QString CHAT_INIT_URL = "https://talkgadget.google.com/u/0/talkgadget/_/chat";
static QString user_agent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/34.0.1847.132 Safari/537.36";

//Timeout to send for setactiveclient requests:
static int ACTIVE_TIMEOUT_SECS = 300;
//Minimum timeout between subsequent setactiveclient requests:
static int SETACTIVECLIENT_LIMIT_SECS = 30;
static QString endpoint = "https://clients6.google.com/chat/v1/";
static QString ORIGIN_URL = "https://talkgadget.google.com";

#define TIMEOUT_MS 10000
#define TIMEOUT_MS_UPLOAD 100000

QString Client::getSelfChatId()
{
    return myself.chat_id;
}

void Client::parseSelfConversationState(QList<MessageField> stateFields, Conversation &res)
{
    //skip 6 fields
    //Self read state
    auto ssstate = stateFields[6].list();
    //Entry 0 is my ID, not so interesting
    //This is the ts representing last time I read
    QString ts = ssstate[1].number();
    //qDebug() << "ssstate, ts: " << ts;

    res.lastReadTimestamp = QDateTime::fromMSecsSinceEpoch(ts.toLongLong() / 1000);
    //qDebug() << res.lastReadTimestamp.toString();
    //todo: parse this

    res.status = (ConversationStatus)stateFields[7].number().toInt();
    //TODO: Handle this
    //qDebug() << "STATUS " << status;
    //UNKNOWN_CONVERSATION_STATUS = 0
    //INVITED = 1
    //ACTIVE = 2
    //LEFT = 3

    res.notifLevel = (NotificationLevel)stateFields[8].number().toInt();
    //qDebug() << res.notifLevel;

    auto views = stateFields[9].list();
    //qDebug() << "W size " << views.size();   SIZE IS ALWAYS 1
    res.view = (ConversationView)views[0].number().toInt();
    //UNKNOWN_CONVERSATION_VIEW = 0
    //INBOX_VIEW = 1
    //ARCHIVED_VIEW = 2


    auto inviters_ids = stateFields[10].list();
    QString inviter_id = inviters_ids[0].string();
    //qDebug() << inviter_id;
    res.creator.chat_id = inviter_id;
    QString invitation_ts = stateFields[11].number();
    //qDebug() << invitation_ts;
    res.creation_ts.fromMSecsSinceEpoch(invitation_ts.toUInt());
    QString sort_ts = stateFields[12].number();
    //qDebug() << sort_ts;
    QString a_ts = stateFields[13].number();
    //qDebug() << a_ts;
    //skip 4 fields
}

User Client::parseEntity(QList<MessageField> entity)
{
    User res;
    if (entity.size() >= 10) {
        auto ids = entity[8].list();
        if (ids.size() >= 2) {
            res.chat_id = ids[0].string();
            res.gaia_id = ids[1].string();
        }
        else {
            res.chat_id = "NF";
        }
        auto properties = entity[9].list();
        if (properties.size() >= 5) {
            res.display_name = properties[1].string();
            res.first_name = properties[2].string();
            res.photo = properties[3].string();
            auto emails = properties[4].list();
            if (emails.size() >= 1)
                res.email = emails[0].string();
        }
        else {
            qDebug() << "Parsing single entity properties error " << properties.size();
            res.display_name = res.first_name = res.email = "Not found";
            res.photo = "";
        }

        /*
        qDebug() << "ID: " << res.chat_id;
        qDebug() << "DNAME: " << res.display_name;
        qDebug() << "FNAME: " << res.first_name;
        qDebug() << "EMAIL: " << res.email;
        */
    }
    else {
        qDebug() << "Parsing single entity error " << entity.size();
        res.chat_id = res.gaia_id = res.display_name = res.first_name = res.email = "NF";
        res.photo = "";
    }
    return res;
}

QList<User> Client::parseClientEntities(QList<MessageField> entities)
{
    //qDebug() << "Parsing client entities " << input;
    QList<User> res;
    for (auto messageField : entities) {
        User tmpUser = parseEntity(messageField.list());
        if (!getUserById(tmpUser.chat_id).alreadyParsed)
            res.append(tmpUser);
        //else //qDebug() << "User already in " << tmpUser.chat_id;
    }
    return res;
}

QList<User> Client::parseGroup(QList<MessageField> group)
{
//    qDebug() << "Parsinggroup ";// << input;
    QList<User> res;
    if (group.size() >= 3) {
        auto groupEntities = group[2].list();
        for (auto entity : groupEntities) {
            auto eentity = entity.list();
            if (eentity.size()) {
                User tmpUser = parseEntity(eentity[0].list());
                if (!getUserById(tmpUser.chat_id).alreadyParsed)
                    res.append(tmpUser);
            }
        }
    }
    return res;
}

QList<User> Client::parseUsers(const QString& userString)
{
    QList<User> res;
    QStringRef dsData = Utils::extractArrayForDS(userString, 21);
    int idx = 0;
    auto parsedData = MessageField::parseListRef(dsData, idx);
    //qDebug() << parsedData.size();
    if (parsedData.size() >= 3) {
        auto entities = parsedData[2].list();
        res.append(parseClientEntities(entities));
        //Now we have groups; what are these?
        for (int i = 4; i < parsedData.size(); ++i) {
            res.append(parseGroup(parsedData[i].list()));
        }
    }
    return res;
}

User Client::getUserById(QString chatId)
{
    //qDebug() << "Searching for user " << chatId;
    foreach (User u, users)
        if (u.chat_id==chatId) {
            u.alreadyParsed = true;
            return u;
        }

    User foo;
    foo.alreadyParsed = false;
    foo.gaia_id = foo.chat_id = chatId;
    foo.display_name = "User not found";
    return foo;
}

QString Client::getConversationName(QString convId)
{
    return rosterModel->getConversationName(convId);
}

User Client::getEntityById(QString cid) {
    User foo;
    foo.chat_id = foo.gaia_id = cid;
    foo.display_name = "User not found 2";
    foo.first_name = "Unknown";
    foo.email = "foo@foo.foo";
    foo.alreadyParsed = true;
    foo.photo = "";
    if (!connectedToInternet) {
        contactsModel->addContact(foo);
        return foo;
    }

    QString body = "[";
    body += getRequestHeader();
    body += ", null, [[\"";
    body += cid;
    body += "\"]]]";
    qDebug() << body;
    QNetworkReply *reply = sendRequest("contacts/getentitybyid",body);

    //Sync on te request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    qDebug() << "Got reply!";

    timeoutTimer.stop();
    QString sreply = reply->readAll();
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200) {
        qDebug() << "200";
        int idx = 0;
        auto parsedReply = MessageField::parseListRef(sreply.leftRef(-1), idx);
        if (parsedReply.size() < 3) {
            qDebug() << "Too short";
            reply->deleteLater();
            contactsModel->addContact(foo);
            return foo;
        }
        auto entities = parsedReply[2].list();
        if (entities.size() != 1) {
            qDebug() << "Something went wrong, have " << entities.size();
        }
        else {
            //qDebug() << "Parsing";
            //qDebug() << sreply;
            auto entity = entities[0].list();
            User tmp = parseEntity(entity);
            //qDebug() << "Parsed";
            if (tmp.chat_id == "NF") {
                tmp.chat_id = tmp.gaia_id = cid;
            }
            //Some users may have no info like display_name or whatsoever... Probably these have deleted their g+ account
            if (tmp.display_name == "")
                tmp.display_name = "Unknown";
            if (tmp.first_name == "")
                tmp.first_name = "Unknown";
            qDebug() << "Corrected";
            contactsModel->addContact(tmp);
            qDebug() << "Added";
            reply->deleteLater();
            qDebug() << "DeletedLater";
            return tmp;
        }
    }
    contactsModel->addContact(foo);
    reply->deleteLater();
    return foo;
}

void Client::parseConversationAbstract(QList<MessageField> abstractFields, Conversation &res)
{
    //First we have ID -- ignore

    //Then type
    if (abstractFields.size() < 2)
        return;
    QString type = abstractFields[1].number();
    //qDebug() << "TYPE: " << type;
    //Name (optional) -- need to see what happens when it is set
    if (abstractFields.size() < 3)
        return;
    res.name = abstractFields[2].string();
    //qDebug() << "Name: " << res.name;

    // parse the state
    if (abstractFields.size() < 4)
        return;
    auto conversationState = abstractFields[3].list();
    parseSelfConversationState(conversationState, res);

    if (abstractFields.size() < 8)
        return;
    //Now I have read_state
    QList<ReadState> readStates;
    for (auto rs : abstractFields[7].list()) {
        readStates << Utils::parseReadState(rs);
    }

    if (abstractFields.size() < 13)
        return;
    //skip 4 fields
    auto participants = abstractFields[12].list();
    for (auto p : participants) {
        Identity id = Utils::parseIdentity(p.list());
        User tmp = getUserById(id.chat_id);

        if ((tmp.chat_id == myself.chat_id) || (tmp.display_name != "User not found")) {
            res.participants.append({tmp, {}});
        }
        else {
            if (tmp.chat_id != myself.chat_id) {
                qDebug() << "Have a UNF here " << tmp.chat_id;
                User parsedUser = getEntityById(tmp.chat_id);
                //Put it anyway, as a reminder that we should update it
                qDebug() << "Returned";
                res.participants.append({parsedUser, {}});
                qDebug() << "Appended";
            }
        }
    }

    //Merge read states with participants
    //THIS HOLDS INFO ONLY ABOUT MYSELF, NOT NEEDED NOW!
    foreach (Participant p, res.participants)
    {
        foreach (ReadState r, readStates)
        {
            if (p.user.chat_id == r.userid.chat_id)
            {
                p.last_read_timestamp = r.last_read;
                //qDebug() << p.last_read_timestamp.toString();
                break;
            }
        }
    }
}

Conversation Client::parseConversation(QList<MessageField> conversation)
{
    Conversation res;
    if (!conversation.size())
        return res;
    QList<MessageField> cconversation = conversation[0].list();
    if (!cconversation.size())
        return res;
    res.id = cconversation[0].string();


    // Abstract
    if (conversation.size() < 2)
        return res;
    auto abstract = conversation[1].list();
    parseConversationAbstract(abstract, res);
    // Events
    if (conversation.size() < 3)
        return res;
    auto details = conversation[2].list();
    for (auto event : details) {
        Event e = Utils::parseEvent(event.list());
        if (e.notificationLevel != 0 && e.notificationLevel != res.notifLevel) {
            //qDebug() << "Setting notifLev to " << e.notificationLevel;
            //This can be QUIET although the conversation is not
            //res.notifLevel = (NotificationLevel)e.notificationLevel;
        }

        if (e.value.segments.size() > 0 || e.value.attachments.size()>0) //skip empty messages (voice calls)
            res.events.append(e);
    }
    return res;
}

void Client::parseConversationState(MessageField conv)
{
    //qDebug() << "CONV_STATE: ";// << conv;

    Conversation c = parseConversation(conv.list());
    qDebug() << c.id;
    //qDebug() << c.events.size();

    //Now formalize what happened:

    //New messages for existing convs?
    if (!rosterModel->conversationExists(c.id)) {
        //A new conv was created!
        //TODO: test this case
        rosterModel->addConversationAbstract(c);
    }

    int newValidEvts = 0, validIdx=0;
    bool diffConv = false;
    int i=0;

    foreach (Event e, c.events) {
        //Eventually update the notificationLevel for this conversation
        rosterModel->updateNotificationLevel(e.conversationId, e.notificationLevel);
        //skip empty messages (voice calls)
        if ((e.value.segments.size() > 0 || e.value.attachments.size()>0)) {
            qDebug() << "I have a valid msg here";
            bool added = conversationModel->addEventToConversation(c.id, e);
            //I should put on top the conversation anyway
            rosterModel->putOnTop(c.id);
            //If I'm not the active client these messages have already been read probably
            if (added && !notifier->isAnotherClientActive())
                rosterModel->addUnreadMsg(c.id);
            newValidEvts++;
            if (i+1 < c.events.size() && c.events[i].conversationId != c.events[i+1].conversationId)
                diffConv = true;
            validIdx = i;
        }
        i++;
    }

    if (newValidEvts > 1) {
        //More than 1 new message -> show a generic notification
        if (diffConv) {
            qDebug() << "Notif many, diffConv";
            emit showNotification(QString(QString::number(newValidEvts) + " new messages"), "Restored channel", "You have new msgs", "Many conversations", newValidEvts, c.id);
        }
        else {
            qDebug() << "Notif many, NO diffConv";
            emit  showNotification(QString(QString::number(newValidEvts) + " new messages"), "Restored channel", "You have new msgs", rosterModel->getConversationName(c.events[0].conversationId), newValidEvts, c.id);
        }
    }
    else if (newValidEvts == 1 && c.events[validIdx].notificationLevel != QUIET) {
        //Only 1 new message -> show a specific notification
        emit showNotification(c.events[validIdx].value.segments[validIdx].value, c.events[validIdx].sender.chat_id, c.events[validIdx].value.segments[validIdx].value, c.events[validIdx].sender.chat_id, 1, c.id);
    }
    else {
        qDebug() << "Not notifying";
        qDebug() << "New msgs: " << newValidEvts;
    }
}

QList<Conversation> Client::parseConversations(const QString& conv)
{
    QList<Conversation> res;
    QStringRef dsData = Utils::extractArrayForDS(conv, 19);
    int idx = 0;
    auto parsedData = MessageField::parseListRef(dsData, idx);
    //Skip 3 fields
    if (parsedData.size() >= 4) {
        auto conversations = parsedData[3].list();
        for (auto conversation : conversations) {
            res.append(parseConversation(conversation.list()));
        }
    }

    /*
    //Debug info
    foreach (Conversation c, res) {
        qDebug() << "###### CONV #######";
        qDebug() << c.id;
        qDebug() << c.participants.size();
        foreach (Participant p, c.participants) {
            qDebug() << p.user.chat_id;
        }
    }
    */
    return res;
}

void Client::postReply(QNetworkReply *reply)
{
    timeoutTimer.stop();
    if (reply->error() == QNetworkReply::NoError) {
        //qDebug() << "No errors on Post";
    }
}

User Client::parseMySelf(const QString& sreply) {
    //SELF INFO
    QStringRef dsData = Utils::extractArrayForDS(sreply, 20);
    int idx = 0;
    auto parsedData = MessageField::parseListRef(dsData, idx);
    User res;
    //And then parse a common user
    if (parsedData.size() >= 3) {
        auto entity = parsedData[2].list();

        res = parseEntity(entity);

        /*
        qDebug() << res.chat_id;
        qDebug() << res.display_name;
        qDebug() << res.first_name;
        qDebug() << res.photo;
        */
    }
    return res;
}

void Client::networkReply()
{
    timeoutTimer.stop();

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    qDebug() << "DBG Got " << c.size() << "from" << reply->url();
    foreach(QNetworkCookie cookie, c) {
        for (int i=0; i<sessionCookies.size(); i++) {
            if (sessionCookies[i].name() == cookie.name()) {
                qDebug() << "Updating cookie " << cookie.name();
                sessionCookies[i].setValue(cookie.value());
            }
        }
    }
    if (reply->error() == QNetworkReply::NoError) {
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==302) {
            qDebug() << "Redir";
            QVariant possibleRedirectUrl =
                    reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
            followRedirection(possibleRedirectUrl.toUrl());
            reply->deleteLater();
        }
        else {
            QString sreply = reply->readAll();
            //qDebug() << sreply;

            //API KEY
            QStringRef dsData = Utils::extractArrayForDS(sreply, 7);
            int idx = 0;
            auto ds7Parsed = MessageField::parseListRef(dsData, idx);
            api_key = ds7Parsed[2].string();
            qDebug() << "API KEY: " << api_key;
            if (api_key.contains("AF_initDataKeys") || api_key.size() < 10 || api_key.size() > 50) {
                qDebug() << sreply;
                qDebug() << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                //exit(-1);
                qDebug() << "Auth expired!";
                //Not smart for sure, but it should be safe
                deleteCookies();
            }

            // Channel info & header_id
            dsData = Utils::extractArrayForDS(sreply, 4);
            idx = 0;
            auto ds4Parsed = MessageField::parseListRef(dsData, idx);
            channel_path = ds4Parsed[1].string();
            channel_ec_param = ds4Parsed[4].string().remove("\\n").remove('\\');
            channel_prop_param = ds4Parsed[5].string();
            header_id = ds4Parsed[7].string();

            // Header info
            dsData = Utils::extractArrayForDS(sreply, 2);
            idx = 0;
            auto ds2Parsed = MessageField::parseListRef(dsData, idx);
            header_date = ds2Parsed[4].string();
            header_version = ds2Parsed[6].string();

            qDebug() << "HID " << header_id;
            qDebug() << "HDA " << header_date;
            qDebug() << "HVE " << header_version;

            myself = parseMySelf(sreply);
            if (!myself.email.contains("@"))
            {
                qDebug() << "Error parsing myself info";
                qDebug() << sreply;
                //initChat();
                //return;
            }
            rosterModel->setMySelf(myself);

            //Parse users!
            users = parseUsers(sreply);
            foreach (User u, users)
                contactsModel->addContact(u);

            //Parse conversations
            QList<Conversation> convs = parseConversations(sreply);
            foreach (Conversation c, convs)
            {
                c.unread = 0;
                foreach (Event e, c.events)
                    if (e.timestamp > c.lastReadTimestamp)
                        c.unread++;
                rosterModel->addConversationAbstract(c);
                conversationModel->addConversation(c);
            }
            reply->deleteLater();
            emit(initFinished());
        }
    }
    else {
        //failure
        //qDebug() << "Failure" << reply->errorString();
        emit authFailed("Login Failed " + reply->errorString());
        reply->deleteLater();
    }
}

QByteArray Client::getAuthHeader()
{
    QByteArray res = "SAPISIDHASH ";
    qint64 time_msec = QDateTime::currentMSecsSinceEpoch();//1000;
    QString auth_string = QString::number(time_msec);
    auth_string += " ";
    foreach (QNetworkCookie cookie, sessionCookies) {
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

void Client::uploadPerformedReply()
{
    timeoutTimer.stop();

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    qDebug() << "Got " << c.size() << "from" << reply->url();
    foreach(QNetworkCookie cookie, c) {
        qDebug() << cookie.name();
    }
    QString sreply = reply->readAll();
    qDebug() << "Response " << sreply;
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200) {
        qDebug() << "Upload in progress";
        QJsonDocument doc = QJsonDocument::fromJson(sreply.toUtf8());
        QJsonObject obj = doc.object().take("sessionStatus").toObject();
        QString state = obj.take("state").toString();
        qDebug() << state;
        if (state=="FINALIZED") {
            //Completed, send the message!

            //Retrieve the id of the image
            QString imgId = obj.take("additionalInfo").toObject()
                    .take("uploader_service.GoogleRupioAdditionalInfo").toObject()
                    .take("completionInfo").toObject()
                    .take("customerSpecificInfo").toObject()
                    .take("photoid").toString();
            qDebug() << "Sending msg with img" << imgId;

            sendImageMessage(outgoingImages.at(0).conversationId, imgId, "");
            outgoingImages.clear();
        }
    }
    else {
        qDebug() << "Problem uploading";
    }
    reply->deleteLater();
}

void Client::performImageUpload(QString url)
{
    OutgoingImage oi = outgoingImages.at(0);

    QFile inFile(oi.filename);
    if (!inFile.open(QIODevice::ReadOnly)) {
        qDebug() << "File not found";
        return ;
    }

    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", user_agent.toLocal8Bit().data());
    req.setRawHeader("X-GUploader-Client-Info", "mechanism=scotty xhr resumable; clientVersion=82480166");
    req.setRawHeader("content-type", "application/x-www-form-urlencoded;charset=utf-8");
    req.setRawHeader("Content-Length", QByteArray::number(inFile.size()));

    QList<QNetworkCookie> reqCookies;
    foreach (QNetworkCookie cookie, sessionCookies) {
        if (cookie.name()=="SAPISID" || cookie.name()=="SSID" || cookie.name()=="HSID" || cookie.name()=="APISID" || cookie.name()=="SID")
            reqCookies.append(cookie);
    }
    req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(reqCookies));
    QNetworkReply * reply = nam->post(req, inFile.readAll());
    imageBeingUploaded = reply;
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(uploadPerformedReply()));
    QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(slotError(QNetworkReply::NetworkError)));
    //QObject::connect(this, SIGNAL(cancelAllActiveRequests()), reply, SLOT(abort()));
    if (!timeoutTimer.isActive())
        timeoutTimer.start(TIMEOUT_MS_UPLOAD);
}

void Client::uploadImageReply()
{
    timeoutTimer.stop();

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    qDebug() << "Got " << c.size() << "from" << reply->url();
    foreach(QNetworkCookie cookie, c) {
        qDebug() << cookie.name();
    }
    QString sreply = reply->readAll();
    qDebug() << "Response " << sreply;
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200) {
        qDebug() << "Upload ready";
        QVariant v = reply->header(QNetworkRequest::LocationHeader);
        QString uploadUrl = qvariant_cast<QString>(v);
        qDebug() << uploadUrl;
        reply->close();
        reply->deleteLater();
        delete reply;
        performImageUpload(uploadUrl);
    }
    else {
        qDebug() << "Problem uploading " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }
    reply->deleteLater();
}

void Client::sendMessageReply() {
    timeoutTimer.stop();

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    pendingRequests.remove(reply);

    qDebug() << reply;
    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    qDebug() << "Got " << c.size() << "from" << reply->url();
    foreach(QNetworkCookie cookie, c) {
        qDebug() << cookie.name();
    }
    qDebug() << "Response " << reply->readAll();
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200) {
        qDebug() << "Message sent correctly";
        Event evt;
        conversationModel->addSentMessage(reply, "convId", evt);
        emit messageSent();
    }
    else {
        qDebug() << "Problem sending msg";
        Event evt;
        conversationModel->addErrorMessage(reply, "convId", evt);
        emit messageNotSent();
    }
    reply->deleteLater();
}

QNetworkReply *Client::sendRequest(QString function, QString json) {
    QString url = endpoint + function;
    //add params
    QNetworkRequest req(url);
    req.setRawHeader("authorization", getAuthHeader());
    req.setRawHeader("x-origin", QVariant::fromValue(ORIGIN_URL).toByteArray());
    req.setRawHeader("x-goog-authuser", "0");
    req.setRawHeader("content-type", "application/json+protobuf");
    QByteArray postDataSize = QByteArray::number(json.size());
    //req.setRawHeader("Content-Length", postDataSize);

    QList<QNetworkCookie> reqCookies;
    foreach (QNetworkCookie cookie, sessionCookies) {
        if (cookie.name()=="SAPISID" || cookie.name()=="SSID" || cookie.name()=="HSID" || cookie.name()=="APISID" || cookie.name()=="SID")
            reqCookies.append(cookie);
    }
    req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(reqCookies));
    url += "?alt=json&key=";
    url += api_key;
    QByteArray postData;
    //postData.append("{\"alt\": \"json\", \"key\": \"" + api_key + "\"}");
    postData.append(json);
    if (!timeoutTimer.isActive())
        timeoutTimer.start(TIMEOUT_MS);
    //qDebug() << "Sending request " << url;
    return nam->post(req, postData);
    //qDebug() << "req sent " << postData;
}

QString Client::getRequestHeader()
{
    QString res = "[[3, 3, \"";
    res += header_version;
    res += "\", \"";
    res += header_date;
    res += "\"], [\"";
    res += clid;
    res += "\", \"";
    res += header_id;
    res += "\"], null, \"en\"]";
    return res;
}

void Client::sendImageMessage(QString convId, QString imgId, QString segments)
{
    if (!connectedToInternet)
        return;

    rosterModel->putOnTop(convId);

    QString seg = "[[0, \"";
    seg += segments;
    seg += "\", [0, 0, 0, 0], [null]]]";
    //qDebug() << "Sending cm " << segments;

    if (segments=="") seg = "[]";

    //Not really random, but works well
    qint64 time = QDateTime::currentMSecsSinceEpoch();
    uint random = (uint)(time % qint64(4294967295u));
    QString body = "[";
    body += getRequestHeader();
    //qDebug() << "gotH " << body;
    body += ", null, null, null, [], [";
    body += seg;
    body += ", []], [[\"";
    body += imgId;
    body += "\", false]], [[\"";
    body += convId;
    body += "\"], ";
    body += QString::number(random);
    body += ", 2, [1]], null, null, null, []]";
    //Eventually body += ", 2, [1]], ["chatId",null,null,null,null,[]], null, null, []]";
    qDebug() << "gotH " << body;
    QNetworkReply *reply = sendRequest("conversations/sendchatmessage",body);
    pendingRequests.insert(reply);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(sendMessageReply()));
    QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(slotError(QNetworkReply::NetworkError)));
}

void Client::sendChatMessage(QString segments, QString conversationId) {
    if (!connectedToInternet)
        return;

    //originalText is needed to push the outgoing event in the correct form
    qDebug() << segments;
    QString originalText = segments;

    rosterModel->putOnTop(conversationId);

    QString seg = "[[0, \"";
    //Correct quotes
    seg += segments.replace('"', "\\\"");
    seg += "\", [0, 0, 0, 0], [null]]]";
    //qDebug() << "Sending cm " << segments;

    //Not really random, but works well
    qint64 time = QDateTime::currentMSecsSinceEpoch();
    uint random = (uint)(time % qint64(4294967295u));
    QString body = "[";
    body += getRequestHeader();
    //qDebug() << "gotH " << body;
    body += ", null, null, null, [], [";
    body += seg;
    body += ", []], null, [[\"";
    body += conversationId;
    body += "\"], ";
    body += QString::number(random);
    body += ", 2], null, null, null, []]";
    qDebug() << "gotH " << body;

    Event outgoingEvt;
    outgoingEvt.sender.chat_id = myself.chat_id;
    outgoingEvt.sender.gaia_id = myself.gaia_id;
    outgoingEvt.conversationId = conversationId;
    outgoingEvt.value.valid = true;
    EventValueSegment evs;
    evs.type = 0;
    evs.value = originalText;
    outgoingEvt.value.segments.append(evs);

    QNetworkReply *reply = sendRequest("conversations/sendchatmessage",body);
    conversationModel->addOutgoingMessage(reply, conversationId, outgoingEvt);
    pendingRequests.insert(reply);

    QObject::connect(reply, SIGNAL(finished()), this, SLOT(sendMessageReply()));
    QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(slotError(QNetworkReply::NetworkError)));
}

void Client::sendImage(QString segments, QString conversationId, QString filename)
{
    QFile inFile(filename);
    if (!inFile.open(QIODevice::ReadOnly)) {
        qDebug() << "File not found";
        return ;
    }

    OutgoingImage oi;
    oi.conversationId = conversationId;
    oi.filename = filename;
    outgoingImages.append(oi);

    //First upload image to gdocs
    QJsonObject emptyObj;
    QJsonArray jarr;
    QJsonObject j1, jj1;
    j1["name"] = QJsonValue(QString("file"));
    j1["filename"] = QJsonValue(QString(filename.right(filename.size()-filename.lastIndexOf("/")-1)));
    j1["put"] = emptyObj;
    j1["size"] =  QJsonValue(inFile.size());
    jj1["external"] = j1;

    QJsonObject j2, jj2;
    j2["name"] = QJsonValue(QString("album_mode"));
    j2["content"] = QJsonValue(QString("temporary"));
    j2["contentType"] = QJsonValue(QString("text/plain"));
    jj2["inlined"] = j2;

    QJsonObject j3, jj3;
    j3["name"] = QJsonValue(QString("title"));
    j3["content"] = QJsonValue(QString(filename.right(filename.size()-filename.lastIndexOf("/")-1)));
    j3["contentType"] = QJsonValue(QString("text/plain"));
    jj3["inlined"] = j3;

    qint64 time = QDateTime::currentMSecsSinceEpoch();
    QJsonObject j4, jj4;
    j4["name"] = QJsonValue(QString("addtime"));
    j4["content"] = QJsonValue(QString::number(time));
    j4["contentType"] = QJsonValue(QString("text/plain"));
    jj4["inlined"] = j4;

    QJsonObject j5, jj5;
    j5["name"] = QJsonValue(QString("batchid"));
    j5["content"] = QJsonValue(QString::number(time));
    j5["contentType"] = QJsonValue(QString("text/plain"));
    jj5["inlined"] = j5;

    QJsonObject j6, jj6;
    j6["name"] = QJsonValue(QString("album_name"));
    j6["content"] = QJsonValue(QString("hangish"));
    j6["contentType"] = QJsonValue(QString("text/plain"));
    jj6["inlined"] = j6;

    QJsonObject j7, jj7;
    j7["name"] = QJsonValue(QString("album_abs_position"));
    j7["content"] = QJsonValue(QString("0"));
    j7["contentType"] = QJsonValue(QString("text/plain"));
    jj7["inlined"] = j7;

    QJsonObject j8, jj8;
    j8["name"] = QJsonValue(QString("client"));
    j8["content"] = QJsonValue(QString("hangouts"));
    j8["contentType"] = QJsonValue(QString("text/plain"));
    jj8["inlined"] = j8;

    jarr.append(jj1);
    jarr.append(jj2);
    jarr.append(jj3);
    jarr.append(jj4);
    jarr.append(jj5);
    jarr.append(jj6);
    jarr.append(jj7);
    jarr.append(jj8);

    QJsonObject jjson;
    jjson["fields"] = jarr;

    QJsonObject json;
    json["createSessionRequest"] = jjson;
    json["protocolVersion"] = QJsonValue(QString("0.8"));


    //url += "?alt=json&key=";
    //url += api_key;
    qDebug() << "Sending request for up image";
    QJsonDocument doc(json);
    qDebug() << doc.toJson();

    QNetworkRequest req(QUrl("https://docs.google.com/upload/photos/resumable?authuser=0"));
    //req.setRawHeader("authorization", getAuthHeader());
    req.setRawHeader("User-Agent", user_agent.toLocal8Bit().data());
    req.setRawHeader("X-GUploader-Client-Info", "mechanism=scotty xhr resumable; clientVersion=82480166");
    req.setRawHeader("content-type", "application/x-www-form-urlencoded;charset=utf-8");
    req.setRawHeader("Content-Length", QByteArray::number(doc.toJson().size()));

    QList<QNetworkCookie> reqCookies;
    foreach (QNetworkCookie cookie, sessionCookies) {
        if (cookie.name()=="SAPISID" || cookie.name()=="SSID" || cookie.name()=="HSID" || cookie.name()=="APISID" || cookie.name()=="SID")
            reqCookies.append(cookie);
    }
    req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(reqCookies));
    QNetworkReply * reply = nam->post(req, doc.toJson());
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(uploadImageReply()));
    QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(slotError(QNetworkReply::NetworkError)));
    //QObject::connect(this, SIGNAL(cancelAllActiveRequests()), reply, SLOT(abort()));
    if (!timeoutTimer.isActive())
        timeoutTimer.start(TIMEOUT_MS);
}

void Client::syncAllNewEventsReply()
{
    timeoutTimer.stop();

    //The content of this reply contains CLIENT_CONVERSATION_STATE, such as lost messages
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    qDebug() << "Got " << c.size() << "from" << reply->url();
    foreach(QNetworkCookie cookie, c) {
        qDebug() << cookie.name();
    }
    QString sreply = reply->readAll();
    qDebug() << "Response " << sreply;
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200) {
        qDebug() << "Synced correctly";
        syncAllNewEventsString += sreply;
        int idx = 0;
        auto parsedReply = MessageField::parseListRef(syncAllNewEventsString.leftRef(-1), idx);
        //Skip csanerp:         parsedReply[0]
        //Skip response header: parsedReply[1]
        //Skip sync_timestamp:  parsedReply[2]
        //Parse the actual data
        if (parsedReply.size() < 4) {
            reply->deleteLater();
            return;
        }
        auto cstates = parsedReply[3].list();
        for (auto state : cstates) {
            qDebug() << "Parsing cstate";
            parseConversationState(state);
        }
        needSync = false;
    }
    reply->deleteLater();
}


void Client::syncAllNewEventsDataArrval()
{
    timeoutTimer.stop();

    //The content of this reply contains CLIENT_CONVERSATION_STATE, such as lost messages
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    qDebug() << "Got " << c.size() << "from" << reply->url();
    foreach(QNetworkCookie cookie, c) {
        qDebug() << cookie.name();
    }
    QString sreply = reply->readAll();
    qDebug() << "Response " << sreply;
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200) {
        qDebug() << "Syncing correctly";
        syncAllNewEventsString += sreply;
    }
}


void Client::setPresence(bool goingOffline)
{
    if (!connectedToInternet)
        return;

    //TODO: find the right value for the last parameter, 40 is active desktop
    //... other values fail to set presence as online

    QDateTime now = QDateTime::currentDateTime();
    if (lastSetPresence.addSecs(SETACTIVECLIENT_LIMIT_SECS) > now)
        return;
    lastSetPresence = now;

    QString body = "[";
    body += getRequestHeader();
    body += ",[";
    body += QString::number(720);
    //1 should be mobile (smartphone), 0 offline
    if (goingOffline)
        body += ", 0]]";
    else
        body += ", 40]]";
    qDebug() << body;
    QNetworkReply *reply = sendRequest("presence/setpresence",body);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(setPresenceReply()));
    QObject::connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(slotError(QNetworkReply::NetworkError)));
}

void Client::retrieveConversationLog(QString convId)
{
    if (!connectedToInternet)
        return;

    qDebug() << "Retr history";
    int maxEvents = 20;
    QString body = "[";
    body += getRequestHeader();
    body += ", [[\"";
    body += convId;
    body += "\"], [], []],1,1,null,";
    body += QString::number(maxEvents);
    body += ", [null,null,";
    body += QString::number(conversationModel->getFirstEventTs(convId) .toMSecsSinceEpoch()*1000);
    body += "]]";
    qDebug() << body;
    QNetworkReply *reply = sendRequest("conversations/getconversation",body);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(retrieveConversationLogReply()));
    QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(slotError(QNetworkReply::NetworkError)));
}


void Client::leaveConversation(QString convId)
{
    if (!connectedToInternet)
        return;

    conversationBeingRemoved = convId;

    qint64 time = QDateTime::currentMSecsSinceEpoch();
    uint random = (uint)(time % qint64(4294967295u));

    qDebug() << "Leaving conversation";
    QString body = "[";
    body += getRequestHeader();
    body += ", null, null, null,[[\"";
    body += convId;
    body += "\"], ";
    body += QString::number(random);
    body += ",2]]";
    qDebug() << body;
    QNetworkReply *reply = sendRequest("conversations/removeuser",body);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(leaveConversationReply()));
    QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(slotError(QNetworkReply::NetworkError)));
}

void Client::leaveConversationReply() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    qDebug() << "Got " << c.size() << "from" << reply->url();
    foreach(QNetworkCookie cookie, c) {
        qDebug() << cookie.name();
    }
    QString sreply = reply->readAll();
    qDebug() << "Response " << sreply;
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200) {
        qDebug() << "Left correctly";
        rosterModel->deleteConversation(conversationBeingRemoved);
    }
    else {
        qDebug() << "Couldn't leave conversation";
    }
    reply->deleteLater();
}

void Client::deleteConversation(QString convId)
{
    if (!connectedToInternet)
        return;

    conversationBeingRemoved = convId;

    qint64 time = QDateTime::currentMSecsSinceEpoch();

    qDebug() << "Leaving conversation";
    QString body = "[";
    body += getRequestHeader();
    body += ", [\"";
    body += convId;
    body += "\"], ";
    body += QString::number(time);
    body += ",null, [], ]";
    qDebug() << body;
    QNetworkReply *reply = sendRequest("conversations/deleteconversation",body);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(deleteConversationReply()));
    QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(slotError(QNetworkReply::NetworkError)));
}

void Client::deleteConversationReply() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    qDebug() << "Got " << c.size() << "from" << reply->url();
    foreach(QNetworkCookie cookie, c) {
        qDebug() << cookie.name();
    }
    QString sreply = reply->readAll();
    qDebug() << "Response " << sreply;
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200) {
        qDebug() << "Deleted correctly";
        rosterModel->deleteConversation(conversationBeingRemoved);
    }
    else {
        qDebug() << "Couldn't delete conversation";
    }
    reply->deleteLater();
}


void Client::changeNotificationsForConversation(QString convId, int level)
{
    if (!connectedToInternet)
        return;

    //Reuse the same variable, at any given time only one operation on conversations is active
    conversationBeingRemoved = convId;
    notificationLevelBeingSet = level;

    qDebug() << "Changing conversation notification level";
    QString body = "[";
    body += getRequestHeader();
    body += ", [\"";
    body += convId;
    body += "\"], ";
    body += QString::number(level);
    body += "]";
    qDebug() << body;
    qDebug() << body;
    QNetworkReply *reply = sendRequest("conversations/setconversationnotificationlevel",body);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(changeNotificationsForConversationReply()));
    QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(slotError(QNetworkReply::NetworkError)));

}

void Client::changeNotificationsForConversationReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    qDebug() << "Got " << c.size() << "from" << reply->url();
    foreach(QNetworkCookie cookie, c) {
        qDebug() << cookie.name();
    }
    QString sreply = reply->readAll();
    qDebug() << "Response " << sreply;
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200) {
        qDebug() << "Notif level changed correctly";
        rosterModel->updateNotificationLevel(conversationBeingRemoved, notificationLevelBeingSet);
    }
    else {
        qDebug() << "Couldn't change notification level";
    }
    reply->deleteLater();
}

void Client::renameConversation(QString convId, QString newName)
{
    if (!connectedToInternet)
        return;

    //Reuse the same variable, at any given time only one operation on conversations is active
    conversationBeingRemoved = convId;
    convNameBeingSet = newName;

    qint64 time = QDateTime::currentMSecsSinceEpoch();
    uint random = (uint)(time % qint64(4294967295u));

    qDebug() << "Renaming conversation";
    QString body = "[";
    body += getRequestHeader();
    body += ", null, \"";
    body += newName;
    body += "\", null,[[\"";
    body += convId;
    body += "\"], ";
    body += QString::number(random);
    body += ",1]]";
    qDebug() << body;
    QNetworkReply *reply = sendRequest("conversations/renameconversation",body);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(renameConversationReply()));
    QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(slotError(QNetworkReply::NetworkError)));
}

void Client::renameConversationReply() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    qDebug() << "Got " << c.size() << "from" << reply->url();
    foreach(QNetworkCookie cookie, c) {
        qDebug() << cookie.name();
    }
    QString sreply = reply->readAll();
    qDebug() << "Response " << sreply;
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200) {
        qDebug() << "Renamed correctly";
        rosterModel->renameConversation(conversationBeingRemoved, convNameBeingSet);
    }
    else {
        qDebug() << "Couldn't rename conversation";
    }
    reply->deleteLater();
}

void Client::parseConversationLog(MessageField conv)
{
    qDebug() << "CONV_STATE: ";// << conv;

    Conversation c = parseConversation(conv.list());
    qDebug() << c.id;
    qDebug() << c.events.size();

    //Now formalize what happened:

    //New messages for existing convs?
    if (!rosterModel->conversationExists(c.id)) {
        //A new conv? Here?? no way
        return;
    }
    for (int i=c.events.size()-1; i>=0; i--)
    {
        Event e = c.events[i];
        if ((e.value.segments.size() > 0 || e.value.attachments.size()>0) && e.value.valid) //skip empty messages (voice calls)
            conversationModel->addEventToConversation(c.id, e, false);
    }
}

void Client::retrieveConversationLogReply()
{
    timeoutTimer.stop();

    //The content of this reply contains CLIENT_CONVERSATION_STATE, such as lost messages
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
    qDebug() << "Got " << c.size() << "from" << reply->url();
    foreach(QNetworkCookie cookie, c) {
        qDebug() << cookie.name();
    }
    QString sreply = reply->readAll();
    qDebug() << "Response " << sreply;
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200) {
        qDebug() << "Synced correctly";
        int idx = 0;
        auto parsedReply = MessageField::parseListRef(sreply.leftRef(-1), idx);

        //Skip cgcrp:         parsedReply[0]
        //Skip response header: parsedReply[1]
        //Parse the actual data
        parseConversationLog(parsedReply[2]);
    }
    reply->deleteLater();
}

void Client::setPresenceReply()
{
    timeoutTimer.stop();

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QString sreply = reply->readAll();
    qDebug() << "Set presence response " << sreply;
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()!=200) {
        qDebug() << "There was an error setting presence! " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }
    reply->deleteLater();
}

void Client::setFocusReply()
{
    timeoutTimer.stop();

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QString sreply = reply->readAll();
    qDebug() << "Set focus response " << sreply;
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()!=200) {
        qDebug() << "There was an error setting focus! " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }
    reply->deleteLater();
}

void Client::setFocus(QString convId, int status)
{
    if (!connectedToInternet)
        return;

    QString body = "[";
    body += getRequestHeader();
    body += ", [\"";
    body += convId;
    body += "\"], ";
    body += QString::number(status);
    body += ", 300]";
    qDebug() << body;
    QNetworkReply *reply = sendRequest("conversations/setfocus",body);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(setFocusReply()));
    QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(slotError(QNetworkReply::NetworkError)));
}

void Client::setTyping(QString convId, int status)
{
    if (!connectedToInternet)
        return;

    QString body = "[";
    body += getRequestHeader();
    body += ", [\"";
    body += convId;
    body += "\"], ";
    body += QString::number(status);
    body += "]";
    qDebug() << body;
    QNetworkReply *reply = sendRequest("conversations/settyping",body);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(setTypingReply()));
    QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(slotError(QNetworkReply::NetworkError)));
}

void Client::setTypingReply()
{
    timeoutTimer.stop();

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QString sreply = reply->readAll();
    qDebug() << "Set typing response " << sreply;
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()!=200) {
        qDebug() << "There was an error setting typing status! " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }
    reply->deleteLater();
}

void Client::setActiveClientReply()
{
    timeoutTimer.stop();

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QString sreply = reply->readAll();
    qDebug() << "Set active client response " << sreply;
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()!=200) {
        qDebug() << "There was an error setting active client! " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }
    else {
        //I've just set this; I can assume I am the active client
        notifier->activeClientUpdate(IS_ACTIVE_CLIENT);
    }
    reply->deleteLater();
}

void Client::syncAllNewEvents(QDateTime timestamp)
{
    if (!connectedToInternet)
        return;

    QString body = "[";
    body += getRequestHeader();
    body += ", ";
    body += QString::number(timestamp.toMSecsSinceEpoch() * 1000);
    body += ",[], null, [], 0, [], 1048576]";
    qDebug() << body;
    QNetworkReply *reply = sendRequest("conversations/syncallnewevents",body);
    syncAllNewEventsString = "";
    QObject::connect(reply, SIGNAL(readyRead()), this, SLOT(syncAllNewEventsDataArrval()));
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(syncAllNewEventsReply()));
}

void Client::setActiveClient()
{
    if (!connectedToInternet)
        return;

    QDateTime now = QDateTime::currentDateTime();
    if (lastSetActive.addSecs(SETACTIVECLIENT_LIMIT_SECS) > now)
        return;
    lastSetActive = now;
    QString body = "[";
    body += getRequestHeader();
    body += ", " + QString::number(IS_ACTIVE_CLIENT) +" , \"";
    body += myself.email;
    body += "/";
    body += clid;
    body += "\", ";
    body += QString::number(ACTIVE_TIMEOUT_SECS);
    body += "]";
    qDebug() << body;
    QNetworkReply *reply = sendRequest("clients/setactiveclient",body);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(setActiveClientReply()));
    QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(slotError(QNetworkReply::NetworkError)));
}

void Client::updateWatermarkReply()
{
    timeoutTimer.stop();

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QString sreply = reply->readAll();
    qDebug() << "Update watermark response " << sreply;
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()!=200) {
        qDebug() << "There was an error updating the wm! " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }
    reply->deleteLater();
}

void Client::updateWatermark(QString convId)
{
    if (!connectedToInternet)
        return;

    qDebug() << "Updating wm";
    //If there are no unread messages we can avoid generating data traffic
    if (!rosterModel->hasUnreadMessages(convId))
        return;
    QString body = "[";
    body += getRequestHeader();
    body += ", [";
    body += convId;
    body += "], ";
    body += QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()*1000);
    body += "]";
    QNetworkReply *reply = sendRequest("conversations/updatewatermark",body);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(updateWatermarkReply()));
    QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(slotError(QNetworkReply::NetworkError)));

    rosterModel->setReadConv(convId);
}


void Client::followRedirection(QUrl url)
{
    QNetworkRequest req( url );
    req.setRawHeader("User-Agent", user_agent.toLocal8Bit().data());
    QNetworkReply * reply = nam->get(req);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(networkReply()));
    QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(slotError(QNetworkReply::NetworkError)));
    //QObject::connect(this, SIGNAL(cancelAllActiveRequests()), reply, SLOT(abort()));
    if (!timeoutTimer.isActive())
        timeoutTimer.start(TIMEOUT_MS);
}

void Client::pvtReply()
{
    timeoutTimer.stop();

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200) {
        QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
        QList<QNetworkCookie> c = qvariant_cast<QList<QNetworkCookie> >(v);
        qDebug() << "Got " << c.size() << "cookies from" << reply->url();

        //TODO: make this procedure safe even with more than 1 cookie as answer
        bool updated = false;
        foreach(QNetworkCookie cookie, c) {
            for (int i=0; i<sessionCookies.size(); i++) {
                if (sessionCookies[i].name() == cookie.name()) {
                    qDebug() << "Updating cookie " << sessionCookies[i].name();
                    sessionCookies[i].setValue(cookie.value());
                    updated = true;
                }
            }
        }
        if (!updated)
            sessionCookies.append(c);
        //END OF TODO
        QString rep = reply->readAll();
        reply->close();

        int start = 0;
        auto parsedRep = MessageField::parseListRef(rep.leftRef(-1), start);
        QString pvtToken;
        if (parsedRep.size() > 1)
            pvtToken = parsedRep[1].string();


        if (c.size()>0) {
            auth->updateCookies(sessionCookies);
        }
        else {
            nam->setCookieJar(new QNetworkCookieJar(this));
            initChat(pvtToken);
        }
    }
    else {
        qDebug() << "Pvt req returned " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==0)
        {
            qDebug() << "No network?";
            //exit(0);
            stuckWithNoNetwork = true;
            emit authFailed("No network");
        }
        else {
            qDebug() << "Unk Error";
            //exit(0);
            emit authFailed("Unknown Error " + reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString());
        }
        reply->close();
    }
    reply->deleteLater();
}

void Client::initChat(QString pvt)
{
    QString surl = QString("https://hangouts.google.com/webchat/u/0/load?prop=hangish&fid=gtn-roster-iframe-id&ec=[\"ci:ec\",true,true,false]");
    surl += "&pvt=";
    surl += pvt;
    QUrl url(surl);
    QNetworkRequest req( url );
    req.setRawHeader("User-Agent", user_agent.toLocal8Bit().data());
    req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(sessionCookies));
    QNetworkReply * reply = nam->get(req);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(networkReply()));
    QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(slotError(QNetworkReply::NetworkError)));
    //QObject::connect(this, SIGNAL(cancelAllActiveRequests()), reply, SLOT(abort()));
    if (!timeoutTimer.isActive())
        timeoutTimer.start(TIMEOUT_MS);
}

void Client::getPVTToken()
{
    QNetworkRequest req( QUrl( QString("https://talkgadget.google.com/talkgadget/_/extension-start") ) );
    req.setRawHeader("User-Agent", user_agent.toLocal8Bit().data());

    req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(sessionCookies));
    QNetworkReply * reply = nam->get(req);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(pvtReply()));
    QObject::connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(slotError(QNetworkReply::NetworkError)));
    //QObject::connect(this, SIGNAL(cancelAllActiveRequests()), reply, SLOT(abort()));
    if (!timeoutTimer.isActive())
        timeoutTimer.start(TIMEOUT_MS);
}

void Client::authenticationDone()
{
    sessionCookies = auth->getCookies();
    stuckWithNoNetwork = false;
    nam->setCookieJar(new QNetworkCookieJar(this));
    getPVTToken();
}

void Client::cookieUpdateSlot(QNetworkCookie cookie)
{
    qDebug() << "CLT: upd " << cookie.name();
    for (int i=0; i<sessionCookies.size(); i++) {
        if (sessionCookies[i].name() == cookie.name()) {
            qDebug() << "Updating cookie " << sessionCookies[i].name();
            sessionCookies[i].setValue(cookie.value());
        }
    }
    nam->setCookieJar(new QNetworkCookieJar(this));
    //auth->updateCookies(sessionCookies);
}

void Client::catchNotificationForCover(int num) {
    //channel alway receives 1 message
    emit showNotificationForCover(num);
}

void Client::notificationPushedSlot(QString convId) {
    emit notificationPushed(convId);
}

void Client::renameConvSlot(QString convId, QString newname)
{
    rosterModel->renameConversation(convId, newname);
}

void Client::initDone()
{
    channel = new Channel(sessionCookies, channel_path, header_id, channel_ec_param, channel_prop_param, myself, conversationModel, rosterModel);
    QObject::connect(channel, SIGNAL(channelLost()), this, SLOT(channelLostSlot()));
    QObject::connect(channel, SIGNAL(channelRestored(QDateTime)), this, SLOT(channelRestoredSlot(QDateTime)));
    QObject::connect(channel, SIGNAL(isTyping(QString,QString,int)), this, SLOT(isTypingSlot(QString,QString,int)));
    QObject::connect(channel, SIGNAL(updateWM(QString)), this, SLOT(updateWatermark(QString)));
    QObject::connect(channel, SIGNAL(updateClientId(QString)), this, SLOT(updateClientId(QString)));
    QObject::connect(channel, SIGNAL(cookieUpdateNeeded(QNetworkCookie)), this, SLOT(cookieUpdateSlot(QNetworkCookie)));
    QObject::connect(channel, SIGNAL(renameConversation(QString,QString)), this, SLOT(renameConvSlot(QString, QString)));

    notifier = new Notifier(this, contactsModel);
    QObject::connect(this, SIGNAL(showNotification(QString,QString,QString,QString,int,QString)), notifier, SLOT(showNotification(QString,QString,QString,QString,int,QString)));
    QObject::connect(channel, SIGNAL(showNotification(QString,QString,QString,QString,int,QString)), notifier, SLOT(showNotification(QString,QString,QString,QString,int,QString)));
    QObject::connect(notifier, SIGNAL(showNotificationForCover(int)), this, SLOT(catchNotificationForCover(int)));
    QObject::connect(notifier, SIGNAL(deletedNotifications()), this, SLOT(catchDeletedNotifications()));
    QObject::connect(notifier, SIGNAL(notificationPushed(QString)), this, SLOT(notificationPushedSlot(QString)));
    QObject::connect(channel, SIGNAL(activeClientUpdate(int)), notifier, SLOT(activeClientUpdate(int)));

    channel->listen();
    initCompleted = true;
}

void Client::secondFactorNeededSlot()
{
    emit secondFactorNeeded();
}

void Client::loginNeededSlot()
{
    qDebug() << "lneed";
    //Using this bool to avoid sending signal before ui is opened (which causes the signal to be ignored)
    needLogin = true;
}

Client::Client(RosterModel *prosterModel, ConversationModel *pconversationModel, ContactsModel *pcontactsModel)
{
    nam = new QNetworkAccessManager();
    QObject::connect(&timeoutTimer, SIGNAL(timeout()), this, SLOT(networkTimeout()));

    connectedToInternet = true;
    needSync = false;
    needLogin = false;
    initCompleted = false;
    rosterModel = prosterModel;
    conversationModel = pconversationModel;
    contactsModel = pcontactsModel;

    //init tools
    //auth = new Authenticator();
    auth = new OAuth2Auth();
    QObject::connect(auth, SIGNAL(loginNeeded()), this, SLOT(loginNeededSlot()));
    QObject::connect(auth, SIGNAL(gotCookies()), this, SLOT(authenticationDone()));
    QObject::connect(auth, SIGNAL(authFailed(QString)), this, SLOT(authFailedSlot(QString)));

    auth->auth();

    QObject::connect(this, SIGNAL(initFinished()), this, SLOT(initDone()));

    //QObject::connect(conversationModel, SIGNAL(conversationRequested(QString)), this, SLOT(loadConversationModel(QString)));
    QObject::connect(this, SIGNAL(conversationLoaded()), conversationModel, SLOT(conversationLoaded()));
}


void Client::sendCredentials(QString uname, QString passwd)
{
    qDebug() << "Should I still send something?";
    //auth->send_credentials(uname, passwd);
}

void Client::sendAuthCode(QString pin)
{
    //auth->send2ndFactorPin(pin);
    auth->sendAuthRequest(pin);
}

void Client::deleteCookies()
{
    QString homeDir = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QString homePath = homeDir + "/oauth2.json";
    QFile cookieFile(homePath);
    cookieFile.remove();
    QString vwCookiesPath = homeDir + "/.QtWebKit";
    QDir wvdir(vwCookiesPath);
    wvdir.removeRecursively();
    exit(0);
}

void Client::testNotification()
{
    //qDebug() << "Sending test notif";
    emit showNotification("preview", "summary", "body", "sender", 1, "foo");
}

void Client::channelLostSlot()
{
    emit(channelLost());
}

void Client::isTypingSlot(QString convId, QString chatId, int type)
{
    QString uname = contactsModel->getContactFName(chatId);
    emit isTyping(convId, uname, type);
}

void Client::channelRestoredSlot(QDateTime lastRec)
{
    //If there was another pending req use its ts (that should be older)
    if (!needSync) {
        needSync = true;
        needSyncTS = lastRec;
    }
    qDebug() << "Chnl restored, gonna sync with " << lastRec.toString();
    syncAllNewEvents(needSyncTS);
    emit(channelRestored());
}

void Client::connectivityChanged(QString a, QDBusVariant b)
{
    qDebug() << "Conn changed " << a;
    qDebug() << b.variant().toString();
    //Here I know the channel is going to die, so I can speed up things

    // a == Connected -> connectivity change
    // a == Powered -> toggled wifi

    //Ignore "ready": may be sent when connection is online

    if (a=="State") {
        if (b.variant().toString()=="online") {
            if (stuckWithNoNetwork) {
                emit authFailed("Retrying...");
                auth->auth();
                //getPVTToken();
                return;
            }
            channel->setStatus(true);
            channel->fastReconnect();
            connectedToInternet = true;
        }
        else if (b.variant().toString()=="idle" || b.variant().toString()=="offline"){
            //I'm not connected
            channel->setStatus(false);
            connectedToInternet = false;
            emit(channelLost());
        }
    }
}

void Client::forceChannelRestore()
{
    //Ok, channel is dead; don't wait for the timer and try to bring it up again
    channel->fastReconnect();
}

void Client::forceChannelCheckAndRestore()
{
    qDebug() << "Force Checking channel";
    QDateTime last = channel->getLastPushTs();
    QDateTime now = QDateTime::currentDateTime();
    //Checking for 17 secs in order to avoid rounding problems and small delays
    if (last.addSecs(31) < now) {
        qDebug() << "Reactivating";
        forceChannelRestore();
    }
}

void Client::setAppOpened()
{
    if (!appPaused)
        return;

    if (needLogin)
        emit(loginNeeded());

    appPaused = false;
    if (!initCompleted)
        return;

    if (needSync)
       syncAllNewEvents(needSyncTS);

    QString convId = conversationModel->getCid();
    if (convId!="")
        updateWatermark(convId);

    notifier->closeAllNotifications();
    emit deletedNotifications();

    setActiveClient();
    //setPresence(false);
    channel->setAppOpened();
    forceChannelCheckAndRestore();
}

void Client::catchDeletedNotifications()
{
    emit deletedNotifications();
}

void Client::setAppPaused()
{
    if (!initCompleted)
        return;

    if (appPaused)
        return;

    channel->setAppPaused();
    //setPresence(true);
    appPaused = true;
}

OAuth2Auth *Client::getAuthenticator()
{
    return auth;
}

void Client::authFailedSlot(QString error)
{
    emit authFailed(error);
    //No network?
    stuckWithNoNetwork = true;
    emit(channelLost());
}

void Client::updateClientId(QString newID)
{
    qDebug() << "Updating clid " << newID;
    clid = newID;
}

QString Client::getLastIncomingConversationId() {
    return channel->getLastIncomingConversation();
}

QString Client::getLastIncomingConversationName() {
    QString lastId = channel->getLastIncomingConversation();
    if (lastId.size()>1)
        return rosterModel->getConversationName(lastId);
    return "";
}

void Client::deleteMessageWError(QString text)
{
    conversationModel->deleteMsgWError(text);
}

void Client::networkTimeout() {
    qDebug() << "time out in Client";
    timeoutTimer.stop();
    slotError(QNetworkReply::NetworkSessionFailedError);
}

void Client::slotError(QNetworkReply::NetworkError err)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    qDebug() << "Error in client " << err;

    if (reply == imageBeingUploaded) {
        qDebug() << "Image upload failed";
        emit imageUploadFailed();
    }

    //I have error 8 after long inactivity, and the connection can't be reestablished, let's try the following
    //OperationCanceledError is instead triggered by the LPRequst, but if this happens it means that there's an operation stuck for more than 30 seconds; let's try to create a new QNetworkAccessManager and see if it helps
    if (err == QNetworkReply::OperationCanceledError || err==QNetworkReply::NetworkSessionFailedError || err==QNetworkReply::UnknownNetworkError) {
        nam->deleteLater();
        nam = new QNetworkAccessManager();
        //emit qnamUpdated(nam);
        //QNetworkSession session( nam->configuration() );
        qDebug() << "Aborting all " << pendingRequests.size();
        //emit cancelAllActiveRequests();
        QSetIterator<QNetworkReply *> itr(pendingRequests);
        while (itr.hasNext()) {
            qDebug() << "Aborting one";
            Event evt;
            conversationModel->addErrorMessage(itr.next(), "convId", evt);
        }
        pendingRequests.clear();
        //session.close();
        //session.open();
        //nam->setCookieJar(new QNetworkCookieJar(this));
    }
    //I may want to try to submit the original request again at some point, need further investigation
    //reply->request()
}


void Client::testFunction() {
    //A debuggin slot
}
