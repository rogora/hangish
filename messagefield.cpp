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
along with hangish.  If not, see <http://www.gnu.org/licenses/>

*/

#include "messagefield.h"

#include <QtCore/QDebug>

// Parses the list and sets start to the end of the list
QList<MessageField> MessageField::parseListRef(QStringRef text, int &start)
{
    if (text.at(start) != '[' && text.at(start) != '{') {
        qWarning() << "Not a list to parse";
        start += text.size();
        return {};
    }
    int openBrackets = 0;
    int openMapBrackets = 0;
    if (text.at(start)=='[')
        openBrackets++;
    else
        openMapBrackets++;

    QList<MessageField> results;
    for(int i = start + 1; i < text.length(); ++i) {
        MessageField nextMessageField;
        //qDebug() << text.at(i);
        if (text.at(i) == '[') {
            nextMessageField.type_ = List;
            nextMessageField.listValue_ = parseListRef(text, i);
            // move the cursor to the comma
            if (i + 1 < text.length() && text.at(i+1) == ',') ++i;
        }
        else if (text.at(i) == '{') {
            nextMessageField.type_ = Map;
            nextMessageField.mapValue_ = parseMapRef(text, i);
            // move the cursor to the comma
            if (i + 1 < text.length() && text.at(i+1) == ',') ++i;
        }
        else if (text.at(i) == ',') {
            nextMessageField.type_ = Empty;
        }
        else if (text.size() > i+3 && text.at(i)=='n' && text.at(i+1)=='u' && text.at(i+2)=='l' && text.at(i+3)=='l') {
            nextMessageField.type_ = Empty;
            i += 3;
            if (i + 1 < text.length() && text.at(i+1) == ',') ++i;
        }
        else if (text.at(i) == '"') {
            nextMessageField.type_ = String;
            nextMessageField.stringValue_ = parseString(text, i);
            // move the cursor to the comma
            if (i + 1 < text.length() && text.at(i+1) == ',') ++i;
        } else if (text.at(i).isDigit()) {
            nextMessageField.type_ = Number;
            nextMessageField.numberValue_ = parseNumber(text, i);
            // move the cursor to the comma
            if (i + 1 < text.length() && text.at(i+1) == ',') ++i;
        } else if (text.at(i) == '\'') {
            nextMessageField.type_ = String;
            nextMessageField.stringValue_ = parseCharString(text, i);
            // move the cursor to the comma
            if (i + 1 < text.length() && text.at(i+1) == ',') ++i;
        } else if (text.at(i) == ']') {
            if (--openBrackets) {
                qWarning() << "Here we shouldn't have any open brackets...";
                Q_ASSERT(false);
            }
            start = i;
            return results;
        }
        else if (text.at(i) == '}') {
            if (--openMapBrackets) {
                qWarning() << "Here we shouldn't have any open brackets...";
                Q_ASSERT(false);
            }
            start = i;
            return results;
        }/*
        else if (i + 3 <= text.length() && text.at(i) == '\\' && text.at(i+1) == '\\' && text.at(i+2) == 'n' && text.at(i+3) == ',') {
            // move the cursor to the comma
            if (i + 1 < text.length() && text.at(i+1) == ',') ++i;
            // don't add garbage to the list:
            i+=3;
            continue;
        }*/
        else if (i + 2 < text.length() && text.at(i) == '\\' && text.at(i+1) == '\\' && text.at(i+2) == 'n') {
            // move the cursor to the comma, if there is one...
            if (i + 3 < text.length() && text.at(i+3) == ',') {
                i += 3;
                continue;
            }
            //... otherwise just skip this newline
            // don't add garbage to the list:
            i += 2;
            continue;
        }
        else if (text.at(i) == '\n') {
            // move the cursor to the comma
            if (i + 1 < text.length() && text.at(i+1) == ',') ++i;
            // don't add garbage to the list:
            continue;
        } else {
            // TODO: we should parse maps they look like this:  { key : value, ... }
            // Image attachments are in maps.
            // For now ignore them: (seems to work fine)
            if (text.at(i)=='\\' || text.at(i)==':')
                continue;
            qWarning() << "Unknown field type????" << text.right(i);
            // in any case we do not want garbage in the list
            //start += text.right(i).size();
            continue;
        }
        results << nextMessageField;
    }
    Q_ASSERT(false);
}

QList<MessageField> MessageField::parseMapRef(QStringRef text, int &start)
{
    return parseListRef(text, start);
}

QStringRef MessageField::parseString(QStringRef text, int& start)
{
    if (text.at(start) != '"') {
        qWarning() << "Not a string to parse";
        return {};
    }
    int escapeInd = -1;
    int strStart = start + 1;
    for (int i = strStart; i < text.length(); ++i) {
        if (text.at(i) == '\\' && escapeInd != i - 1) { // ignore double escapes
            escapeInd = i;
        } else if (text.at(i) == '"') {
            if (i-1 != escapeInd) {
                start = i;
                return text.mid(strStart, i - strStart);
            }
        }
    }
    qWarning() << "no end in string found";
    Q_ASSERT(false);
}

QStringRef MessageField::parseNumber(QStringRef text, int& start)
{
    int i = start;
    while (i < text.length() && text.at(i).isDigit()) ++i;
    int nrStart = start;
    start = i-1;
    return text.mid(nrStart, i-nrStart);
}

QStringRef MessageField::parseCharString(QStringRef text, int &start)
{
    if (text.at(start) != '\'') {
        qWarning() << "Not a char string to parse";
        return {};
    }
    int escapeInd = -1;
    int strStart = start + 1;
    for (int i = start + 1; i < text.length(); ++i) {
        if (text.at(i) == '\\') {
            escapeInd = i;
        } else if (text.at(i) == '\'') {
            if (i-1 != escapeInd) {
                start = i;
                return text.mid(strStart, i - strStart);
            }
        }
    }
    qWarning() << "no end in char string found";
    Q_ASSERT(false);
}
