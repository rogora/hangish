#include "messagefield.h"

#include <QtCore/QDebug>

// Parses the list and sets start to the end of the list
QList<MessageField> MessageField::parseList(QString text, int& start)
{
    if (text[start] != '[') {
        qWarning() << "Not a list to parse";
        return {};
    }
    int openBrackets = 1;
    QList<MessageField> results;
    for(int i = start + 1; i < text.length(); ++i) {
        MessageField nextMessageField;
        if (text[i] == '[') {
            nextMessageField.type_ = List;
            nextMessageField.listValue_ = parseList(text, i);
            // move the cursor to the comma
            if (i + 1 < text.length() && text[i+1] == ',') ++i;
        } else if (text[i] == ',') {
            nextMessageField.type_ = Empty;
        } else if (text[i] == '"') {
            nextMessageField.type_ = String;
            nextMessageField.stringValue_ = parseString(text, i);
            // move the cursor to the comma
            if (i + 1 < text.length() && text[i+1] == ',') ++i;
        } else if (text[i].isDigit()) {
            nextMessageField.type_ = Number;
            nextMessageField.numberValue_ = parseNumber(text, i);
            // move the cursor to the comma
            if (i + 1 < text.length() && text[i+1] == ',') ++i;
        } else if (text[i] == '\'') {
            nextMessageField.type_ = String;
            nextMessageField.stringValue_ = parseCharString(text, i);
            // move the cursor to the comma
            if (i + 1 < text.length() && text[i+1] == ',') ++i;
        } else if (text[i] == ']') {
            if (--openBrackets) {
                qWarning() << "Here we shouldn't have any open brackets...";
                Q_ASSERT(false);
            }
            start = i;
            return results;
        } else {
            qWarning() << "Uknown field type????";
        }
        results << nextMessageField;
    }
    Q_ASSERT(false);
}

QString MessageField::parseString(QString text, int& start)
{
    if (text[start] != '"') {
        qWarning() << "Not a string to parse";
        return {};
    }
    int escapeInd = -1;
    QString result = "";
    for (int i = start + 1; i < text.length(); ++i) {
        if (text[i] == '\\') {
            escapeInd = i;
        } else if (text[i] == '"') {
            if (i-1 != escapeInd) {
                start = i;
                return result;
            }
        }
        result.append(text[i]);
    }
    qWarning() << "no end in string found";
    Q_ASSERT(false);
}

QString MessageField::parseNumber(QString text, int& start)
{
    int i = start;
    QString number;
    while (i < text.length() && text[i].isDigit()) {
        number.append(text[i++]);
    }
    start = i-1;
    return number;
}

QString MessageField::parseCharString(QString text, int &start)
{
    if (text[start] != '\'') {
        qWarning() << "Not a char string to parse";
        return {};
    }
    int escapeInd = -1;
    QString result = "";
    for (int i = start + 1; i < text.length(); ++i) {
        if (text[i] == '\\') {
            escapeInd = i;
        } else if (text[i] == '\'') {
            if (i-1 != escapeInd) {
                start = i;
                return result;
            }
        }
        result.append(text[i]);
    }
    qWarning() << "no end in char string found";
    Q_ASSERT(false);
}

