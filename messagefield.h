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

#ifndef MESSAGEFIELD_H
#define MESSAGEFIELD_H


#include <QtCore/QString>
#include <QtCore/QList>

class MessageField
{
    public:
        enum FieldType {Empty, String, Number, List, Map};

        static QList<MessageField> parseListRef(QStringRef text, int& start);
        static QList<MessageField> parseMapRef(QStringRef text, int& start);
        static QStringRef parseString(QStringRef text, int& start);
        static QStringRef parseNumber(QStringRef text, int& start);
        static QStringRef parseCharString(QStringRef text, int& start);

        inline QString string() const;
        inline QString number() const;
        inline QList<MessageField> list() const;
        inline QList<MessageField> map() const;
        inline FieldType type() const;

    private:
        FieldType type_;
        QStringRef stringValue_;
        QStringRef numberValue_;
        QList<MessageField> listValue_;
        QList<MessageField> mapValue_;
};

QString MessageField::string() const { if (type_ == String) return stringValue_.toString(); else return QString();}
QString MessageField::number() const { if (type_ == Number) return numberValue_.toString(); else return QString();}
QList<MessageField> MessageField::list() const { if (type_ == List) return listValue_; else return {}; }
QList<MessageField> MessageField::map() const { if (type_ == Map) return mapValue_; else return {}; }

MessageField::FieldType MessageField::type() const { return type_; }

#endif // MESSAGEFIELD_H
