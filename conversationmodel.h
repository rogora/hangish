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


#ifndef CONVERSATIONMODEL_H
#define CONVERSATIONMODEL_H

#include <QAbstractListModel>
#include "qtimports.h"
#include "structs.h"

class ConversationElement {
public:
    QString text;
    QString sender;
    QString timestamp;
    QString senderId;
    QString fullimageUrl;
    QString previewImageUrl;
    bool read;

    QDateTime ts;

    ConversationElement(QString pSender, QString pSenderId, QString pText, QString pTS, QString pFullImageUrl, QString pPrevImageUrl, bool pRead, QDateTime pts) {
        text = pText;
        sender = pSender;
        senderId = pSenderId;
        timestamp = pTS;
        fullimageUrl = pFullImageUrl;
        previewImageUrl = pPrevImageUrl;
        read = pRead;
        ts = pts;
    }
};

class ConversationModel : public QAbstractListModel
{
    QList<Conversation> conversations;

    Q_OBJECT
    Q_PROPERTY(QString cid READ getCid WRITE loadConversation FINAL)

public:
    void updateReadState(ReadState rs);
    void addConversation(Conversation c);
    void addEventToConversation(QString convId, Event e);
    void addSentMessage(QString convId, Event evt);
    void addOutgoingMessage(QString convId, Event evt);

    QString id, name;

    enum ConversationRoles {
        IdRole = Qt::UserRole + 1,
        SenderRole,
        SenderIdRole,
        TextRole,
        FullImageRole,
        PreviewImageRole,
        TimestampRole,
        ReadRole
    };

    int rowCount(const QModelIndex & parent = QModelIndex()) const;

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

    explicit ConversationModel(QObject *parent = 0);
    void loadConversation(QString cId);
    QString getCid();
    void addConversationElement(QString sender, QString senderId, QString timestamp, QString text, QString fullimageUrl, QString previewimageUrl, bool read, QDateTime pts);

private:
    QString getSenderName(QString chatId, QList<Participant> participants);
    QList<ConversationElement *> myList;

protected:
    QHash<int, QByteArray> roleNames() const;

signals:
    void conversationRequested(QString cid);

public slots:
    void conversationLoaded();

};

#endif // CONVERSATIONMODEL_H
