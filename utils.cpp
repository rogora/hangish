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

#include "messagefield.h"

Utils::Utils()
{
}

QStringRef Utils::extractArrayForDS(const QString &text, int dsKey)
{
    int index = text.indexOf(QString("<script>AF_initDataCallback({key: 'ds:%1'").arg(dsKey));
    index = text.indexOf("return [", index) + strlen("return [");
    int endIndex = text.indexOf("]\n}});</script>", index);
    return text.midRef(index, endIndex-index);
}

int Utils::findPositionFromComma(QString input, int startPos, int commaCount)
{
    int res = startPos;
    for (int i=0; i<commaCount; i++) {
        res = input.indexOf(",", res+1);
    }
    return res;
}

Identity Utils::parseIdentity(const QList<MessageField> &ids)
{
    return {ids[0].stringValue_.toString(), ids[1].stringValue_.toString()};
}

Event Utils::parseEvent(const QList<MessageField>& eventFields)
{
    Event event;
    event.conversationId = eventFields[0].listValue_[0].stringValue_.toString();

    auto ids = eventFields[1].listValue_;
    event.sender = parseIdentity(ids);

    QString tss = eventFields[2].numberValue_.toString();
    qulonglong tsll = tss.toULongLong();
    qint64 ts = (qint64)tsll;
    event.timestamp = QDateTime::fromMSecsSinceEpoch(ts/1000);

    auto self_state = eventFields[3].listValue_;
    // parseNotificationLevel():
    event.notificationLevel = 0;
    if (self_state.size() > 2) {
        event.notificationLevel = self_state[2].numberValue_.toInt();
    }

    // skip 2 ..
    auto eventData = eventFields[6].listValue_;
    if (eventData.isEmpty()) {
       event.value = {false, {}, {}};
    } else {
        // TODO: split here into functions?
        // parseEventValue():
        auto eventValueFields = eventData[2].listValue_;
        auto texts = eventValueFields[0].listValue_;
        // parseTexts():
        for (auto text : texts) {
            // parseEventValueSegment():
            auto textList = text.listValue_;
            int type = textList[0].numberValue_.toInt();
            QString msg = textList[1].stringValue_.toString();
            event.value.segments.append({type, msg});
        }

        auto attachments = eventValueFields[1].listValue_;
        for (auto attachment : attachments) {
            auto attachmentList = attachment.listValue_;
            for (auto innerAttach : attachmentList) {
                auto innerAttachList = innerAttach.listValue_;
                if (innerAttachList.size() >= 3 && innerAttachList[2].type_ == MessageField::List) {
                    innerAttachList = innerAttachList[2].listValue_;
                    if (innerAttachList.size() > 10) {
                        QString fullImageUrl = innerAttachList[5].stringValue_.toString();
                        QString previewUrl = innerAttachList[9].stringValue_.toString();
                        event.value.attachments.append({0, fullImageUrl, previewUrl});
                    }
                }
            }
        }

        event.value.valid = true;
    }
    return event;
}


QString Utils::getChatidFromIdentity(QString identity)
{
    int start = 2;
    int stop = findPositionFromComma(identity, start, 1);
    QString res = identity.mid(start, stop-3);
    qDebug() << "User chatid is " << res;
    return res;
}

ReadState Utils::parseReadState(const MessageField &rs)
{
    ReadState res;
    res.userid = Utils::parseIdentity(rs.listValue_[0].listValue_);
    auto ts = rs.listValue_[1].numberValue_;
    res.last_read = QDateTime::fromMSecsSinceEpoch(ts.toLongLong() / 1000);
    qDebug() << ts;
    qDebug() << res.last_read.toString();
    return res;
}
