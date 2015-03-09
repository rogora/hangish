#ifndef MESSAGEFIELD_H
#define MESSAGEFIELD_H


#include <QtCore/QString>
#include <QtCore/QList>

class MessageField
{
    public:
        enum FieldType {Empty, String, Number, List};

        static QList<MessageField> parseList(QString text, int& start);
        static QString parseString(QString text, int& start);
        static QString parseNumber(QString text, int& start);
        static QString parseCharString(QString text, int& start);

        FieldType type_;
        QString stringValue_;
        QString numberValue_;
        QList<MessageField> listValue_;
};

#endif // MESSAGEFIELD_H
