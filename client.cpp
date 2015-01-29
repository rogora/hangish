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

static QString CHAT_INIT_URL = "https://talkgadget.google.com/u/0/talkgadget/_/chat";
static QString user_agent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/34.0.1847.132 Safari/537.36";
static QString homePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/cookies.json";


//Timeout to send for setactiveclient requests:
static int ACTIVE_TIMEOUT_SECS = 300;
//Minimum timeout between subsequent setactiveclient requests:
static int SETACTIVECLIENT_LIMIT_SECS = 30;
static QString endpoint = "https://clients6.google.com/chat/v1/";
static QString ORIGIN_URL = "https://talkgadget.google.com";

QString Client::getSelfChatId()
{
    return myself.chat_id;
}


Conversation Client::parseSelfConversationState(QString scState, Conversation res)
{
    //qDebug() << scState;
    int start=1;
    //skip 6 fields
    for (int i=0; i<6; i++)
        Utils::getNextAtomicField(scState, start);
    //Self read state
    QString ssstate = Utils::getNextAtomicField(scState, start);
    qDebug() << "ssstate: " << ssstate;
    int sstart = 1;
    //This is my ID, not so interesting
    Utils::getNextAtomicField(ssstate, sstart);
    //This is the ts representing last time I read
    ssstate = Utils::getNextAtomicField(ssstate, sstart);
    qDebug() << "ssstate: " << ssstate;

    res.lastReadTimestamp = QDateTime::fromMSecsSinceEpoch(ssstate.toLongLong() / 1000);
    qDebug() << res.lastReadTimestamp.toString();
    //todo: parse this
    QString status = Utils::getNextAtomicField(scState, start);
    //qDebug() << status;
    QString notificationLevel = Utils::getNextAtomicField(scState, start);
    //qDebug() << notificationLevel;
    QString views = Utils::getNextAtomicField(scState, start);
    //qDebug() << views;
    //TODO:: parse views
    QString inviter_id = Utils::getNextAtomicField(scState, start);
    //qDebug() << inviter_id;
    res.creator.chat_id = inviter_id;
    QString invitation_ts = Utils::getNextAtomicField(scState, start);
    //qDebug() << invitation_ts;
    res.creation_ts.fromMSecsSinceEpoch(invitation_ts.toUInt());
    QString sort_ts = Utils::getNextAtomicField(scState, start);
    //qDebug() << sort_ts;
    QString a_ts = Utils::getNextAtomicField(scState, start);
    //qDebug() << a_ts;
    //skip 4 fields
    for (int i=0; i<4; i++)
        Utils::getNextAtomicField(scState, start);

    return res;
}

User Client::parseEntity(QString input)
{
    User res;
    //qDebug() << "Parsing single entity " << input;
    int start = 1;
    for (int i=0; i<8; i++)
        Utils::getNextAtomicField(input, start);
    Identity id = Utils::parseIdentity(Utils::getNextAtomicField(input, start));
    res.chat_id = id.chat_id;
    res.gaia_id = id.gaia_id;
    qDebug() << "ID: " << id.chat_id;
    QString properties = Utils::getNextAtomicField(input, start);
    start = 1;
    //Skip type
    QString tmp;
    Utils::getNextAtomicField(properties, start);
    //display name
    tmp = Utils::getNextAtomicField(properties, start);
    res.display_name = tmp.mid(1, tmp.size()-2);
    qDebug() << "DNAME: " << res.display_name;
    //first name
    tmp = Utils::getNextAtomicField(properties, start);
    res.first_name = tmp.mid(1, tmp.size()-2);
    qDebug() << "FNAME: " << res.first_name;
    //photo url
    tmp = Utils::getNextAtomicField(properties, start);
    res.photo = tmp.mid(1, tmp.size()-2);

    //emails - this is an array, but i take only the first one now
    QString emails = Utils::getNextAtomicField(properties, start);
    start=1;
    tmp = Utils::getNextAtomicField(emails, start);
    qDebug() << "EMAIL: " << emails;
    qDebug() << "EMAIL: " << tmp;
    res.email = tmp.mid(1, tmp.size()-2);
    qDebug() << "EMAIL: " << res.email;
    return res;
}

QList<User> Client::parseClientEntities(QString input)
{
    //qDebug() << "Parsing client entities " << input;
    QList<User> res;
    int start = 1;
    for (;;) {
        QString tmp = Utils::getNextAtomicField(input, start);
        if (tmp.size() < 10) break;
        User tmpUser = parseEntity(tmp);
        if (!getUserById(tmpUser.chat_id).alreadyParsed)
            res.append(tmpUser);
        //else //qDebug() << "User already in " << tmpUser.chat_id;
    }
    return res;
}

QList<User> Client::parseGroup(QString input)
{
    //qDebug() << "Parsinggroup " << input;
    QList<User> res;
    int start = 1;
    //skip 2
    for (int i=0; i<2; i++)
        Utils::getNextAtomicField(input, start);

    QString repeatedField = Utils::getNextAtomicField(input, start);
    start = 1;
    for (;;) {
        QString nextEntity = Utils::getNextField(Utils::getNextAtomicField(repeatedField, start), 1);
        if (nextEntity.size() < 30) break;
        User tmpUser = parseEntity(nextEntity);
        if (!getUserById(tmpUser.chat_id).alreadyParsed)
            res.append(tmpUser);
    }
    return res;
}

QList<User> Client::parseUsers(QString userString)
{
    QList<User> res;
    int start = userString.indexOf("</script><script>AF_initDataCallback({key: 'ds:21',");
    start = userString.indexOf("data:[[", start) + strlen("data:[[");
    //Skip 2 fields
    Utils::getNextAtomicField(userString, start);
    Utils::getNextAtomicField(userString, start);
    //Utils::getNextAtomicField(conv, start);
    QString entities = Utils::getNextAtomicField(userString,start);
    res.append(parseClientEntities(entities));
    //Skip 1
    Utils::getNextAtomicField(userString,start);
    //Now we have groups; what are these?
    for (;;) {
        QString group = Utils::getNextAtomicField(userString,start);
        //qDebug() << "Will parse group - " << group.size();
        if (group.size() < 50)
            break;
        res.append(parseGroup(group));
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

Participant Client::parseParticipant(QString plist)
{
    Participant res;
    Identity tmp = Utils::parseIdentity(plist);
    //qDebug() << "Will look for " << tmp.chat_id;
    res.user = getUserById(tmp.chat_id);
    return res;
}


QList<Participant> Client::parseParticipants(QString plist, QString data)
{
    QList<Participant> res;
    int start=1;
    for (;;) {
        QString part = Utils::getNextAtomicField(plist,start);
        if (part.size()<10) break;
        Participant p = parseParticipant(part);
        res.append(p);
    }
    return res;
}

Conversation Client::parseConversationAbstract(QString abstract, Conversation res)
{
    int start = 0;
    start+=1;
    //First we have ID
    QString id = Utils::getNextAtomicField(abstract, start);
    id = id.mid(2, res.id.size());
    //qDebug() << "ID: " << id;
    //Then type
    //res.type = ConversationType(.mid(1,1).toInt());
    QString type = Utils::getNextAtomicField(abstract, start);
    qDebug() << "TYPE: " << type;
    //Name (optional) -- need to see what happens when it is set
    QString name = Utils::getNextAtomicField(abstract, start);
    res.name = name;
    //qDebug() << "Name: " << res.name;
    res = parseSelfConversationState(Utils::getNextAtomicField(abstract, start), res);
    //skip 3 fields
    for (int i=0; i<3; i++)
        Utils::getNextAtomicField(abstract, start);
    //Now I have read_state
    QList<ReadState> readStates = Utils::parseReadStates(Utils::getNextAtomicField(abstract, start));
    //skip 4 fields
    for (int i=0; i<4; i++)
        Utils::getNextAtomicField(abstract, start);
    QString current_participants = Utils::getNextAtomicField(abstract, start);
    QString participants_data    = Utils::getNextAtomicField(abstract, start);
    res.participants = parseParticipants(current_participants, participants_data);

    //Merge read states with participants

    //THIS HOLD INFO ONLY ABOUT MYSELF, NOT NEEDED NOW!
    foreach (Participant p, res.participants)
    {
        foreach (ReadState r, readStates)
        {
            if (p.user.chat_id == r.userid.chat_id)
            {
                p.last_read_timestamp = r.last_read;
                qDebug() << p.last_read_timestamp.toString();
                break;
            }
        }
    }

    return res;
}

Conversation Client::parseConversationDetails(QString conversation, Conversation res)
{
    int start = 1;
    for (;;) {
        QString tmp_event = Utils::getNextAtomicField(conversation, start);
        if (tmp_event.size()<10) break;
        res.events.append(Utils::parseEvent(tmp_event));
    }
    return res;
}

Conversation Client::parseConversation(QString conv, int &start)
{
    Conversation res;
    ////qDebug() << "Conversation: " << conv;
    int ptr=1;
    //First we have ID
    res.id = Utils::getNextAtomicField(conv, ptr);
    res.id = res.id.mid(2, res.id.size()-5);
    //qDebug() << "ID: " << res.id;
    QString abstract = Utils::getNextAtomicField(conv, ptr);
    res = parseConversationAbstract(abstract, res);
    QString details = Utils::getNextAtomicField(conv, ptr);
    res = parseConversationDetails(details, res);
    start += ptr;
    return res;
}

void Client::parseConversationState(QString conv)
{
    qDebug() << "CONV_STATE: " << conv;
    int start = 1;

    Conversation c = parseConversation(conv, start);
    qDebug() << c.id;
    qDebug() << c.events.size();

    //Now formalize what happened:

    //New messages for existing convs?
    if (!rosterModel->conversationExists(c.id)) {
        //A new conv was created!
        //TODO: test this case
        rosterModel->addConversationAbstract(c);
    }
    foreach (Event e, c.events)
        conversationModel->addEventToConversation(c.id, e);

    if (c.events.size() > 1)
        //More than 1 new message -> show a generic notification
        emit showNotification(QString(c.events.size() + "new messages"), "Restored channel", "You have new msgs", "sender");
    else if (c.events.size() == 1 && c.events[0].notificationLevel == 30)
        //Only 1 new message -> show a specific notification
        emit showNotification(c.events[0].value.segments[0].value, c.events[0].sender.chat_id, c.events[0].value.segments[0].value, c.events[0].sender.chat_id);
}

QList<Conversation> Client::parseConversations(QString conv)
{
    QList<Conversation> res;
    int start = conv.indexOf("</script><script>AF_initDataCallback({key: 'ds:19',");
    start = conv.indexOf("data:[[", start) + strlen("data:[[");
    //Skip 3 fields
    for (int i=0; i<3; i++)
        Utils::getNextAtomicField(conv, start);
    QString conversations = Utils::getNextField(conv,start);
    int ptr;
    int st=1;
    for (;;) {
        QString conversation = Utils::getNextAtomicField(conversations,st);
        if (conversation.size()<10)
            break;
        res.append(parseConversation(conversation, ptr));
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
    if (reply->error() == QNetworkReply::NoError) {
        //qDebug() << "No errors on Post";
    }
}

User Client::parseMySelf(QString sreply) {
    //SELF INFO
    int start = sreply.indexOf("AF_initDataCallback({key: 'ds:20'") + strlen("AF_initDataCallback({key: 'ds:20'");
    start = sreply.indexOf("data:", start) + strlen("data:");
    sreply = Utils::getNextField(sreply, start);
    QString self_info = Utils::getNextField(sreply, 1);
    qDebug() << "SI: " << self_info;
    User res;
    start = 1;
    //skip 2
    for (int i=0; i<2; i++)
        Utils::getNextAtomicField(self_info, start);
    //And then parse a common user
    res = parseEntity(Utils::getNextAtomicField(self_info, start));
    qDebug() << res.chat_id;
    qDebug() << res.display_name;
    qDebug() << res.first_name;
    qDebug() << res.photo;

    return res;
}

void Client::networkReply()
{
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
            delete reply;
        }
        else {
            QString sreply = reply->readAll();

            //API KEY
            int start = sreply.indexOf("AF_initDataCallback({key: 'ds:7'") + strlen("AF_initDataCallback({key: 'ds:7'");
            int key_start = sreply.indexOf("client.js\",\"", start) + strlen("client.js\",\"");
            int key_stop = sreply.indexOf("\"", key_start) + 1;
            api_key = sreply.mid(key_start, key_stop - key_start-1);
            qDebug() << "API KEY: " << api_key;
            if (api_key.contains("AF_initDataKeys") || api_key.size() < 10 || api_key.size() > 50 || sreply.contains("Logged")) {
                qDebug() << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                qDebug() << sreply;
                //exit(-1);
                qDebug() << "Auth expired!";
                //Not smart for sure, but it should be safe
                deleteCookies();
            }

            QString tmp;
            start = sreply.indexOf("AF_initDataCallback({key: 'ds:4'");
            start = sreply.indexOf("data:[[", start) + strlen("data:[[");
            //Skip 1
            Utils::getNextAtomicField(sreply, start);
            tmp = Utils::getNextAtomicField(sreply, start);
            channel_path = tmp.mid(1, tmp.size()-2);
            //Skip 2
            for (int i=0; i<2; i++) {
                Utils::getNextAtomicField(sreply, start);
            }
            tmp = Utils::getNextAtomicField(sreply, start);
            channel_ec_param = tmp.mid(1, tmp.size()-3);
            for (int i=0; i<channel_ec_param.size();i++)
                if (channel_ec_param.at(i)=='\\')
                    channel_ec_param.remove(i, 1);
            //qDebug() << "channel ec param is " << channel_ec_param;
            tmp = Utils::getNextAtomicField(sreply, start);
            channel_prop_param = tmp.mid(1, tmp.size()-2);
            Utils::getNextAtomicField(sreply, start);
            tmp = Utils::getNextAtomicField(sreply, start);
            header_id = tmp.mid(1, tmp.size()-2);
            //clid = header_id;

            start = sreply.indexOf("AF_initDataCallback({key: 'ds:2'");
            start = sreply.indexOf("data:[[", start) + strlen("data:[[");
            for (int i=0; i<4; i++) {
                Utils::getNextAtomicField(sreply, start);
            }
            tmp = Utils::getNextAtomicField(sreply, start);
            header_date = tmp.mid(1, tmp.size()-2);
            Utils::getNextAtomicField(sreply, start);
            tmp = Utils::getNextAtomicField(sreply, start);
            header_version = tmp.mid(1, tmp.size()-2);

            //qDebug() << "HID " << header_id;
            //qDebug() << "HDA " << header_date;
            //qDebug() << "HVE " << header_version;

            myself = parseMySelf(sreply);
            if (!myself.email.contains("@"))
            {
                qDebug() << "Error parsing myself info";
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
            delete reply;
            emit(initFinished());
        }
    }
    else {
        //failure
        //qDebug() << "Failure" << reply->errorString();
        delete reply;
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
    delete reply;
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
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(uploadPerformedReply()));

}

void Client::uploadImageReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    qDebug() << "DbG: " << reply;

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
//    delete reply;
}

void Client::sendMessageReply() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

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
        conversationModel->addSentMessage("convId", evt);
        emit messageSent();
    }
    else {
        qDebug() << "Problem sending msg";
        emit messageNotSent();
    }
    delete reply;
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
    res += "\"], [null ,\"";
    res += header_id;
    res += "\"], null, \"en\"]";
    return res;
}

void Client::sendImageMessage(QString convId, QString imgId, QString segments)
{
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
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(sendMessageReply()));
}

void Client::sendChatMessage(QString segments, QString conversationId) {
    QString seg = "[[0, \"";
    seg += segments;
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
    QNetworkReply *reply = sendRequest("conversations/sendchatmessage",body);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(sendMessageReply()));
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
    //Then send the message
}

void Client::syncAllNewEventsReply()
{
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
        //Skip csanerp
        int start = 1;
        Utils::getNextAtomicField(sreply, start);
        //Skip response header
        Utils::getNextAtomicField(sreply, start);
        //Skip sync_timestamp
        Utils::getNextAtomicField(sreply, start);
        //Parse the actual data
        QString cstates = Utils::getNextAtomicField(sreply, start);
        start = 1;
        for (;;) {
            QString cstate = Utils::getNextAtomicField(cstates, start);
            qDebug() << cstate;
            if (cstate.size()<10) break;
                parseConversationState(cstate);
        }
        needSync = false;
    }
    delete reply;

}

/*
void Client::syncAllNewEventsDataArrval()
{

}
*/

void Client::setPresence(bool goingOffline)
{
    QString body = "[";
    body += getRequestHeader();
    body += ", [720, ";
    if (goingOffline)
        body += "1";
    else
        body += "40";
    body += "], null, null, [";
    if (goingOffline)
        body += "1";
    else
        body += "0";
    body += "], []]";
    qDebug() << body;
    QNetworkReply *reply = sendRequest("presence/setpresence",body);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(setPresenceReply()));
}

void Client::setPresenceReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QString sreply = reply->readAll();
    qDebug() << "Set presence response " << sreply;
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()!=200) {
        qDebug() << "There was an error setting presence! " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }
}

void Client::setFocusReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QString sreply = reply->readAll();
    qDebug() << "Set focus response " << sreply;
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()!=200) {
        qDebug() << "There was an error setting focus! " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }
}

void Client::setFocus(QString convId, int status)
{
    QString body = "[";
    body += getRequestHeader();
    body += ", [\"";
    body += convId;
    body += "\"], ";
    body += QString::number(status);
    body += ", 20]";
    qDebug() << body;
    QNetworkReply *reply = sendRequest("conversations/setfocus",body);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(setFocusReply()));
}

void Client::setTyping(QString convId, int status)
{
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
}

void Client::setTypingReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QString sreply = reply->readAll();
    qDebug() << "Set typing response " << sreply;
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()!=200) {
        qDebug() << "There was an error setting typing status! " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }
}

void Client::setActiveClientReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    QString sreply = reply->readAll();
    qDebug() << "Set active client response " << sreply;
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()!=200) {
        qDebug() << "There was an error setting active client! " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }
    else {
        notifier->activeClientUpdate(1);
    }
}

void Client::syncAllNewEvents(QDateTime timestamp)
{
    QString body = "[";
    body += getRequestHeader();
    body += ", ";
    body += QString::number(timestamp.toMSecsSinceEpoch() * 1000);
    body += ",[], null, [], 0, [], 1048576]";
    qDebug() << body;
    QNetworkReply *reply = sendRequest("conversations/syncallnewevents",body);
    //I shouldn't need this
    //QObject::connect(reply, SIGNAL(readyRead()), this, SLOT(syncAllNewEventsDataArrval()));

    QObject::connect(reply, SIGNAL(finished()), this, SLOT(syncAllNewEventsReply()));
}

void Client::setActiveClient()
{
    QDateTime now = QDateTime::currentDateTime();
    if (lastSetActive.addSecs(SETACTIVECLIENT_LIMIT_SECS) > now)
        return;
    lastSetActive = now;
    QString body = "[";
    body += getRequestHeader();
    body += ", 1, \"";
    body += myself.email;
    body += "/";
    body += clid;
    body += "\", ";
    body += QString::number(ACTIVE_TIMEOUT_SECS);
    body += "]";
    qDebug() << body;
    QNetworkReply *reply = sendRequest("clients/setactiveclient",body);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(setActiveClientReply()));
}

void Client::updateWatermarkReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    QString sreply = reply->readAll();
    qDebug() << "Update watermark response " << sreply;
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()!=200) {
        qDebug() << "There was an error updating the wm! " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }
}

void Client::updateWatermark(QString convId)
{
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
    qDebug() << body;
    QNetworkReply *reply = sendRequest("conversations/updatewatermark",body);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(updateWatermarkReply()));

    rosterModel->setReadConv(convId);
}


void Client::followRedirection(QUrl url)
{
    QNetworkRequest req( url );
    req.setRawHeader("User-Agent", user_agent.toLocal8Bit().data());
    QNetworkReply * reply = nam->get(req);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(networkReply()));
}

void Client::pvtReply()
{
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
        int start = 1;
        Utils::getNextAtomicField(rep, start);
        QString pvt = Utils::getNextAtomicField(rep, start);
        QString pvtToken;
        if (pvt.size() > 10)
            pvtToken = pvt.mid(1, pvt.size()-2);

        reply->close();
        delete reply;
        //Why is this causing a segfault? Anyway, QT should destroy this...
        //nam->deleteLater();
        nam = new QNetworkAccessManager();
        if (c.size()>0)
            auth->updateCookies(sessionCookies);
        else
            initChat(pvtToken);
    }
    else {
        qDebug() << "Pvt req returned " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        reply->close();
        delete reply;
    }

}

void Client::initChat(QString pvt)
{
    QString surl = QString("https://talkgadget.google.com/u/0/talkgadget/_/chat?prop=hangish&fid=gtn-roster-iframe-id&ec=[\"ci:ec\",true,true,false]");
    surl += "&pvt=";
    surl += pvt;
    QUrl url(surl);
    QNetworkRequest req( url );
    req.setRawHeader("User-Agent", user_agent.toLocal8Bit().data());
    req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(sessionCookies));
    QNetworkReply * reply = nam->get(req);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(networkReply()));
}

void Client::getPVTToken()
{
    QNetworkRequest req( QUrl( QString("https://talkgadget.google.com/talkgadget/_/extension-start") ) );
    req.setRawHeader("User-Agent", user_agent.toLocal8Bit().data());

    req.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(sessionCookies));
    QNetworkReply * reply = nam->get(req);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(pvtReply()));
}

void Client::authenticationDone()
{
    sessionCookies = auth->getCookies();
    getPVTToken();
}

void Client::initDone()
{
    channel = new Channel(nam, sessionCookies, channel_path, header_id, channel_ec_param, channel_prop_param, myself, conversationModel, rosterModel);
    QObject::connect(channel, SIGNAL(channelLost()), this, SLOT(channelLostSlot()));
    QObject::connect(channel, SIGNAL(channelRestored(QDateTime)), this, SLOT(channelRestoredSlot(QDateTime)));
    QObject::connect(channel, SIGNAL(isTyping(QString,QString,int)), this, SLOT(isTypingSlot(QString,QString,int)));
    QObject::connect(channel, SIGNAL(updateWM(QString)), this, SLOT(updateWatermark(QString)));
    QObject::connect(channel, SIGNAL(updateClientId(QString)), this, SLOT(updateClientId(QString)));

    notifier = new Notifier(this, contactsModel);
    QObject::connect(this, SIGNAL(showNotification(QString,QString,QString,QString)), notifier, SLOT(showNotification(QString,QString,QString,QString)));
    QObject::connect(channel, SIGNAL(showNotification(QString,QString,QString,QString)), notifier, SLOT(showNotification(QString,QString,QString,QString)));
    QObject::connect(channel, SIGNAL(activeClientUpdate(int)), notifier, SLOT(activeClientUpdate(int)));

    channel->listen();
    initCompleted = true;
}

void Client::loginNeededSlot()
{
    qDebug() << "lneed";
    //Using this bool to avoid sending signal before ui is opened (which causes the signal to be ignored)
    needLogin = true;
}

Client::Client(RosterModel *prosterModel, ConversationModel *pconversationModel, ContactsModel *pcontactsModel)
{
    needSync = false;
    needLogin = false;
    nam = new QNetworkAccessManager();
    initCompleted = false;
    rosterModel = prosterModel;
    conversationModel = pconversationModel;
    contactsModel = pcontactsModel;

    //init tools
    auth = new Authenticator();
    QObject::connect(auth, SIGNAL(loginNeeded()), this, SLOT(loginNeededSlot()));
    QObject::connect(auth, SIGNAL(gotCookies()), this, SLOT(authenticationDone()));
    QObject::connect(auth, SIGNAL(authFailed(QString)), this, SLOT(authFailedSlot(QString)));

    auth->auth();

    QObject::connect(this, SIGNAL(initFinished()), this, SLOT(initDone()));

    //QObject::connect(conversationModel, SIGNAL(conversationRequested(QString)), this, SLOT(loadConversationModel(QString)));
    QObject::connect(this, SIGNAL(conversationLoaded()), conversationModel, SLOT(conversationLoaded()));

    nam->setCookieJar(&cJar);
}


void Client::sendCredentials(QString uname, QString passwd)
{
    auth->send_credentials(uname, passwd);
}

void Client::send2ndFactorPin(QString pin)
{
    auth->send2ndFactorPin(pin);
}

void Client::deleteCookies()
{
    QFile cookieFile(homePath);
    cookieFile.remove();
    exit(0);
}

void Client::testNotification()
{
    //qDebug() << "Sending test notif";
    emit showNotification("preview", "summary", "body", "sender");
}

void Client::channelLostSlot()
{
    emit(channelLost());
}

void Client::isTypingSlot(QString convId, QString chatId, int type)
{
    QString uname = contactsModel->getContactFName(Utils::getChatidFromIdentity(chatId));
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
    qDebug() << b.variant();
    //Here I know the channel is going to die, so I can speed up things

    // a == Connected -> connectivity change
    // a == Powered -> toggled wifi

    if (a=="Connected" && b.variant().toBool()==true)
        channel->fastReconnect();
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
    if (last.addSecs(17) < now) {
        qDebug() << "Reactivating";
        forceChannelRestore();
    }
}

void Client::setAppOpened()
{
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

    setActiveClient();
    //setPresence(false);
    channel->setAppOpened();
    forceChannelCheckAndRestore();
}

void Client::setAppPaused()
{
    if (!initCompleted)
        return;
    channel->setAppPaused();
    //setPresence(true);
    appPaused = true;
}

void Client::authFailedSlot(QString error)
{
    emit authFailed(error);
}

void Client::updateClientId(QString newID)
{
    qDebug() << "Updating clid " << newID;
    clid = newID;
}
