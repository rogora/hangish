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

QString Utils::cleanText(QString text, QString newLineReplacer)
{
    QString result;
    int size = text.size();
    for (int i = 0; i < size; ++i) {
        if (text.at(i) == '\\' && i+1 < size) {
            if (text.at(i+1) == '\"') {
                ++i;
                result.append('\"');
                continue;
            } else if (text.at(i+1) == '\\') {
                if (i+2 < size && text.at(i+1) == 'n') {
                    result.append("<br>");
                    i+=2;
                    continue;
                }
                ++i;
                result.append('\\');
                continue;
            } else if (text.at(i+1) == 'u') {
                if (i + 5 < size) { // unicode
                    QString valueString = text.mid(i+2, 4);
                    result.append(valueString.toUShort(nullptr, 16));
                }
                i+=5;
                continue;
            } else if (text.at(i+1) == 'n') {
                if (!newLineReplacer.isEmpty()) result.append(newLineReplacer);
                ++i; continue;
            }
        }
        result.append(text.at(i));
    }
    return result;
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
    return {ids[0].string(), ids[1].string()};
}

Event Utils::parseEvent(const QList<MessageField>& eventFields)
{
    Event event;
    event.conversationId = eventFields[0].list()[0].string();

    auto ids = eventFields[1].list();
    event.sender = parseIdentity(ids);

    QString tss = eventFields[2].number();
    qulonglong tsll = tss.toULongLong();
    qint64 ts = (qint64)tsll;
    event.timestamp = QDateTime::fromMSecsSinceEpoch(ts/1000);

    auto self_state = eventFields[3].list();
    // parseNotificationLevel():
    event.notificationLevel = 0;
    if (self_state.size() > 2) {
        event.notificationLevel = self_state[2].number().toInt();
    }

    // skip 2 ..
    auto eventData = eventFields[6].list();
    if (eventData.isEmpty()) {
       event.value = {false, {}, {}};
    } else {
        // TODO: split here into functions?
        // parseEventValue():
        auto eventValueFields = eventData[2].list();
        auto texts = eventValueFields[0].list();
        // parseTexts():
        for (auto text : texts) {
            // parseEventValueSegment():
            auto textList = text.list();
            int type = textList[0].number().toInt();
            QString msg = textList[1].string();
            msg = cleanText(msg, "<br>");
            if (type == 2) { // linke
                msg = QString("<a href=\"%1\">%1</a>").arg(msg);
            }
            event.value.segments.append({type, msg});
        }

        auto attachments = eventValueFields[1].list();
        for (auto attachment : attachments) {
            auto attachmentList = attachment.list();
            for (auto innerAttach : attachmentList) {
                auto innerAttachList = innerAttach.list();
                if (innerAttachList.size() >= 3 && innerAttachList[2].type() == MessageField::List) {
                    innerAttachList = innerAttachList[2].list();
                    if (innerAttachList.size() > 10) {
                        QString fullImageUrl = innerAttachList[5].string();
                        QString previewUrl = innerAttachList[9].string();
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
    return res;
}

ReadState Utils::parseReadState(const MessageField &rs)
{
    ReadState res;
    res.userid = Utils::parseIdentity(rs.list()[0].list());
    auto ts = rs.list()[1].number();
    res.last_read = QDateTime::fromMSecsSinceEpoch(ts.toLongLong() / 1000);
    return res;
}
