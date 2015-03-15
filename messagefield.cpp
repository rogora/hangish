#include "messagefield.h"

#include <QtCore/QDebug>

// Parses the list and sets start to the end of the list
QList<MessageField> MessageField::parseListRef(QStringRef text, int &start)
{
    if (text.at(start) != '[') {
        qWarning() << "Not a list to parse";
        return {};
    }
    int openBrackets = 1;
    QList<MessageField> results;
    for(int i = start + 1; i < text.length(); ++i) {
        MessageField nextMessageField;
        if (text.at(i) == '[') {
            nextMessageField.type_ = List;
            nextMessageField.listValue_ = parseListRef(text, i);
            // move the cursor to the comma
            if (i + 1 < text.length() && text.at(i+1) == ',') ++i;
        } else if (text.at(i) == ',') {
            nextMessageField.type_ = Empty;
        } else if (text.at(i) == '"') {
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
        } else if (text.at(i) == '\n') {
            // move the cursor to the comma
            if (i + 1 < text.length() && text.at(i+1) == ',') ++i;
            // don't add garbage to the list:
            continue;
        } else {
            // TODO: we should parse maps they look like this:  { key : value, ... }
            // Image attachments are in maps.
            // For now ignore them: (seems to work fine)
            if (text.at(i) == '{' || text.at(i) == ':' || text.at(i) == '}') continue;
            qWarning() << "Uknown field type????" << text.right(i);
            // in any case we do not want garbage in the list
            continue;
        }
        results << nextMessageField;
    }
    Q_ASSERT(false);
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
