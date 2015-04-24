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

        inline QString string() const;
        inline QString number() const;
        inline QList<MessageField> list() const;
        inline FieldType type() const;

    private:
        FieldType type_;
        QStringRef stringValue_;
        QStringRef numberValue_;
        QList<MessageField> listValue_;
};

QString MessageField::string() const { if (type_ == String) return stringValue_.toString(); else return QString();}
QString MessageField::number() const { if (type_ == Number) return numberValue_.toString(); else return QString();}
QList<MessageField> MessageField::list() const { if (type_ == List) return listValue_; else return {}; }
MessageField::FieldType MessageField::type() const { return type_; }

#endif // MESSAGEFIELD_H
