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


#ifndef UTILS_H
#define UTILS_H

#include "qtimports.h"
#include "structs.h"
#include "notification.h"

class Utils
{
public:
    Utils();
    static Identity parseIdentity(QString input);
    static int skipTextFields(QString input, int startPos);
    static int skipFields(QString input, int startPos, bool parseBracketsInString = true);
    static int skipFieldsForPush(QString input, int startPos);
    static QString getNextAtomicField(QString conv, int &start, bool parseBracketsInString = true);
    static QString getNextAtomicFieldForPush(QString conv, int &start);
    static QString getNextField(QString conv, int start);
    static QString getTextAtomicField(QString conv, int &start);
    static int findPositionFromComma(QString input, int startPos, int commaCount);

    static Event parseEvent(QString conv);
    static EventValueSegment parseEventValueSegment(QString segment);
    static QList<EventValueSegment> parseTexts(QString segments);
    static EventAttachmentSegment parseEventValueAttachment(QString att);
    static QList<EventAttachmentSegment> parseAttachments(QString attachments);
    static EventValue parseEventValue(QString input);
    static QString getFullUrlFromImageAttach(QString data);
    static QString getPreviewUrlFromImageAttach(QString data);

    static QString getChatidFromIdentity(QString identity);

    static int parseNotificationLevel(QString input);

    static int parseActiveClientUpdate(QString input, QString &newId);

    static ReadState parseReadState(QString input);
    static QList<ReadState> parseReadStates(QString input);
    static ReadState parseReadStateNotification(QString input);
};

#endif // UTILS_H
