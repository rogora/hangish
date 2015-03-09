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

class MessageField;

class Utils
{
public:
    Utils();
    static QStringRef extractArrayForDS(const QString& text, int dsKey);
    static Identity parseIdentity(const QList<MessageField> &ids);
    static int findPositionFromComma(QString input, int startPos, int commaCount);
    static Event parseEvent(const QList<MessageField> &eventFields);
    static QString getChatidFromIdentity(QString identity);
    static ReadState parseReadState(const MessageField &rs);
};

#endif // UTILS_H
