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


#include "utils.h"

Utils::Utils()
{
}

QString Utils::getTextAtomicField(QString conv, int &start)
{
    int stop = skipTextFields(conv, start);
    if (stop==0) {
        return "";
    }
    QString res = conv.mid(start, stop-start);
    //WORKAROUND, FIX ME
//    if (res.endsWith("\""))
  //      res = res.left(res.size()-1);

    start = stop;
    res = res.remove('\\');
    if (!res.contains('"'))
        //Not a string, must be a newline
        return "";
    return res;
}


QString Utils::getNextAtomicField(QString conv, int &start)
{
    int stop = skipFields(conv, start);
    if (stop-start==0) {
        return "";
    }
    QString res = conv.mid(start, stop-start);
    if (res.size() > 1 && res.at(res.size()-1)==',')
        res = res.left(res.size()-1);
    start = stop;
    //qDebug() << "RET " << res;
    return res;
}

QString Utils::getNextAtomicFieldForPush(QString conv, int &start)
{
    int stop = skipFieldsForPush(conv, start);
    if (stop==0) {
        return "";
    }
    QString res = conv.mid(start, stop-start);
    start = stop;
    return res;
}

QString Utils::getNextField(QString conv, int start)
{
    int stop = skipFields(conv, start);
    if (stop-start==0) {
        return "";
    }
    QString res = conv.mid(start, stop-start);
    if (res.size() > 1 && res.at(res.size()-1)==',')
        res = res.left(res.size()-1);
    start = stop;
    //qDebug() << "RET " << res;
    return res;
}

int Utils::skipTextFields(QString input, int startPos)
{
    qDebug() << "TEXT: " << input;
    int res = startPos;
    int last = startPos;
    int obrk, cbrk, quotes;
    obrk = cbrk = quotes = 0;
    for (; ;) {
        res = input.indexOf(",", res);
        if (res==-1) {
            return input.lastIndexOf("]");
        }
        for (int j=last; j<res; j++) {
            if (input.at(j)=='[') obrk++;
            if (input.at(j)==']') cbrk++;
            if (input.at(j)=='"' && (j==input.lastIndexOf('"') || j==input.indexOf('"'))) quotes++;
        }
        //Workaround: when received from channel I have already removed backslashes from input!
        if (obrk == cbrk && ((quotes % 2) == 0) && !(input.size() - res > strlen(",[0,0,0,0] \n,[]\n]")))
        {
            return res;
        }
        last = res;
        res++;
    }
}

int Utils::skipFields(QString input, int startPos)
{
    //Check for empty field

    int res = startPos;
    int last = startPos;
    int obrk, cbrk;
    obrk = cbrk = 0;

    for (; ;) {
        res = input.indexOf(",", res);
        if (res==-1) {
            return input.lastIndexOf("]");
        }
        for (int j=last; j<res; j++) {
            if (input.at(j)=='[') obrk++;
            if (input.at(j)==']') cbrk++;
        }
        if (obrk == cbrk)
        {
            return res+1;
        }
        last = res;
        res++;
    }
}

int Utils::skipFieldsForPush(QString input, int startPos)
{
    //Check for empty field
    int res = startPos;
    int last = startPos;
    int obrk, cbrk;
    obrk = cbrk = 0;

    for (; ;) {
        res = input.indexOf(",", res);
        if (res==-1) {
            return input.lastIndexOf("]");
        }
        for (int j=last; j<res; j++) {
            if (input.at(j)=='[') obrk++;
            if (input.at(j)==']') cbrk++;
            if (obrk == cbrk)
            {
                return j+1;
            }

        }
        last = res;
        res++;
    }
}

int Utils::findPositionFromComma(QString input, int startPos, int commaCount)
{
    int res = startPos;
    for (int i=0; i<commaCount; i++) {
        res = input.indexOf(",", res+1);
    }
    return res;
}

Identity Utils::parseIdentity(QString input)
{
    Identity res;
    int start = 1;
    QString tmp = Utils::getNextAtomicField(input, start);
    res.chat_id = tmp.mid(1, tmp.size()-2);
    tmp = Utils::getNextAtomicField(input, start);
    res.gaia_id = tmp.mid(1, tmp.size()-2);
    return res;
}

EventValueSegment Utils::parseEventValueSegment(QString segment)
{
    EventValueSegment res;
    int start = 1;
    res.type = getNextAtomicField(segment, start).toUInt();
    QString tmp = getTextAtomicField(segment, start);
   /* WORKAROUND
    * int lastQuote = tmp.lastIndexOf('"');
    if (lastQuote > 0)
        res.value = tmp.mid(1, lastQuote - 1);
    else
    */
    res.value = tmp.mid(1, tmp.size() - 2);

    qDebug() << "value found " << res.value;
    return res;
}

QList<EventValueSegment> Utils::parseTexts(QString segments) {
    int start = 1;
    //this is the actual array, loop over it
    QList<EventValueSegment> res;
    for (;;) {
        QString value = getNextAtomicField(segments, start);
        //if (value.size() == 0) break;
        qDebug() << "Segment: " << value;
        //qDebug() << start << " - " << segments.size();
        res.append(parseEventValueSegment(value));
        if (start >= segments.size() - 5) break;
    }
    return res;
}

QString Utils::getFullUrlFromImageAttach(QString data)
{
    qDebug() << data;
    int start = data.indexOf("[[");
    QString res = getNextAtomicField(data, start);
    qDebug() << res;
    start = 1;
    //Skip 5
    for (int i=0; i<5; i++)
        getNextAtomicField(res, start);
    res = getNextAtomicField(res, start);
    qDebug() << res;
    return res.mid(1, res.size()-2);
}

QString Utils::getPreviewUrlFromImageAttach(QString data)
{
    qDebug() << data;
    int start = data.indexOf("[[");
    QString res = getNextAtomicField(data, start);
    qDebug() << res;
    start = 1;
    //Skip 9
    for (int i=0; i<9; i++)
        getNextAtomicField(res, start);
    res = getNextAtomicField(res, start);
    qDebug() << res;
    return res.mid(1, res.size()-2);
}


EventAttachmentSegment Utils::parseEventValueAttachment(QString att)
{
    EventAttachmentSegment res;
    int start = 1;
    res.type = 0;
    qDebug() << getNextAtomicField(att, start); //is this a vector?
    QString data = getNextAtomicField(att, start);
    res.fullImage = getFullUrlFromImageAttach(data);
    res.previewImage = getPreviewUrlFromImageAttach(data);
    qDebug() << res.fullImage;
    qDebug() << res.previewImage;
    return res;
}

QList<EventAttachmentSegment> Utils::parseAttachments(QString attachments){
    int start = 1;
    //array of attchs
    attachments = getNextField(attachments, 1);
    qDebug() << "Attach: " << attachments;
    QList<EventAttachmentSegment> res;
    if (attachments.size() < 1) {
        qDebug() << "ret";
        return res;
    }
    qDebug() << attachments.size();
    for (;;) {
        QString value = getNextAtomicField(attachments, start);
        if (value.size() < 7) break;
        qDebug() << "Attch: " << value;
        res.append(parseEventValueAttachment(value));
    }
    return res;
}

EventValue Utils::parseEventValue(QString input)
{
    EventValue res;
    qDebug() << "Value: " << input;
    int start = 1;
    getNextAtomicField(input, start); //always null?
    getNextAtomicField(input, start); //always []?
    QString content = getNextAtomicField(input, start);
    qDebug() << "CONT: " << content;
    start = 1;
    QString segments = getNextAtomicField(content, start);
    res.segments = parseTexts(segments);
    QString attachments = getNextAtomicField(content, start);
    res.attachments = parseAttachments(attachments);
    qDebug() << "att done";
    return res;
}

int Utils::parseNotificationLevel(QString input)
{
    qDebug() << input;
    int start = 1;
    //userid
    getNextAtomicField(input, start);
    //client_gen_id
    getNextAtomicField(input, start);
    //not level
    QString nl = getNextAtomicField(input, start);
    qDebug() << nl;
    if (nl=="")
        return 0; //Unknown, this is actually possible and used for old messages
    return nl.toInt();
}

Event Utils::parseEvent(QString conv)
{
    int start=1;
    Event res;
    QString conv_id = getNextAtomicField(conv, start);
    qDebug() << conv_id;
    res.conversationId = conv_id.mid(2, conv_id.size()-4);
    QString ids = getNextAtomicField(conv, start);
    res.sender = parseIdentity(ids);
    QString tss = getNextAtomicField(conv, start);
    qulonglong tsll = tss.toULongLong();
    qint64 ts = (qint64)tsll;
    res.timestamp = QDateTime::fromMSecsSinceEpoch(ts/1000);

    //self event state - this hold Notification level -> should we raise a notification for this event?
    QString self_state = getNextAtomicField(conv, start);
    res.notificationLevel = parseNotificationLevel(self_state);

    //skip 2 fields
    for (int i=0; i<2; i++)
        getNextAtomicField(conv, start);
    QString message = getNextAtomicField(conv, start);
    //This is empty in case of a call?
    if (message.size() > 1) {
        res.value = parseEventValue(message);
        res.value.valid = true;
    }
    else
    {
        EventValue ev;
        ev.valid = false;
        res.value = ev;
        qDebug() << "value empty";
    }
    return res;
}


QString Utils::getChatidFromIdentity(QString identity)
{
    int start = 2;
    int stop = findPositionFromComma(identity, start, 1);
    QString res = identity.mid(start, stop-3);
    qDebug() << "User chatid is " << res;
    return res;
}

int Utils::parseActiveClientUpdate(QString input, QString &newId)
{
    newId = "";
    int start = 1;
    QString active_client_state = getNextAtomicField(input, start);
    qDebug() << active_client_state;
    //Skip 6
    for (int i=0; i<6; i++)
        getNextAtomicField(input, start);
    QString client_id = getNextAtomicField(input, start);
    if (client_id.size()>10)
        //Id of the client that triggered the state update
        newId = client_id.mid(1, client_id.size()-2);
    return active_client_state.toInt();
}

ReadState Utils::parseReadState(QString input)
{
    int start = 1;
    ReadState res;
    QString uid = getNextAtomicField(input, start);
    qDebug() << uid;
    res.userid = parseIdentity(uid);
    QString ts = getNextAtomicField(input, start);
    res.last_read = QDateTime::fromMSecsSinceEpoch(ts.toLongLong() / 1000);
    qDebug() << ts;
    qDebug() << res.last_read.toString();
    return res;
}

ReadState Utils::parseReadStateNotification(QString input)
{
    int start = 1;
    ReadState res;
    QString uid = getNextAtomicField(input, start);
    qDebug() << uid;
    res.userid = parseIdentity(uid);
    QString convId = getNextAtomicField(input, start);
    qDebug() << convId;
    //Directly skip [] brackets, without parsing another field
    res.convId = convId.mid(2, convId.size()-4);
    QString ts = getNextAtomicField(input, start);
    res.last_read = QDateTime::fromMSecsSinceEpoch(ts.toLongLong() / 1000);
    qDebug() << ts;
    return res;
}

QList<ReadState> Utils::parseReadStates(QString input)
{
    QList<ReadState> res;
    int start = 1;
    for (;;) {
        QString tmp = getNextAtomicField(input, start);
        if (tmp.size() < 10)
            break;
        res.append(parseReadState(tmp));
    }
    return res;
}
