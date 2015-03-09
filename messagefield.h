#ifndef MESSAGEFIELD_H
#define MESSAGEFIELD_H


#include <QtCore/QString>
#include <QtCore/QList>

class MessageField
{
    public:
        enum FieldType {Empty, String, Number, List};

        static QList<MessageField> parseListRef(QStringRef text, int& start);
        static QStringRef parseString(QStringRef text, int& start);
        static QStringRef parseNumber(QStringRef text, int& start);
        static QStringRef parseCharString(QStringRef text, int& start);

        FieldType type_;
        QStringRef stringValue_;
        QStringRef numberValue_;
        QList<MessageField> listValue_;
};

#endif // MESSAGEFIELD_H
