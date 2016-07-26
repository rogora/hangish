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
    QNetworkReply *id;
    QString text;
    QString sender;
    QString timestamp;
    QString senderId;
    QString fullimageUrl;
    QString previewImageUrl;
    QString video;
    QString uniqueId;
    int type;
    bool read;

    QDateTime ts;

    ConversationElement(QNetworkReply *pId, QString pSender, QString pSenderId, QString pText, QString pTS, QString pFullImageUrl, QString pPrevImageUrl, QString pVideo, bool pRead, QDateTime pts, int pType, QString uid) {
        id = pId;
        text = pText;
        sender = pSender;
        senderId = pSenderId;
        timestamp = pTS;
        fullimageUrl = pFullImageUrl;
        previewImageUrl = pPrevImageUrl;
        video = pVideo;
        read = pRead;
        ts = pts;
        type = pType;
        uniqueId = uid;
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
    bool addEventToConversation(QString convId, Event e, bool bottom=true);
    void addSentMessage(QNetworkReply *id, QString convId, Event evt);
    void addErrorMessage(QNetworkReply *id, QString convId, Event evt);
    void addOutgoingMessage(QNetworkReply *id, QString convId, Event evt);
    QDateTime getFirstEventTs(QString convId);

    QString id, name;

    enum ConversationRoles {
        IdRole = Qt::UserRole + 1,
        SenderRole,
        SenderIdRole,
        TextRole,
        FullImageRole,
        PreviewImageRole,
        VideoRole,
        TimestampRole,
        ReadRole,
        UidRole
    };

    int rowCount(const QModelIndex & parent = QModelIndex()) const;

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

    explicit ConversationModel(QObject *parent = 0);
    void loadConversation(QString cId);
    QString getCid();
    bool addConversationElement(QNetworkReply *id, QString sender, QString senderId, QString timestamp, QString text, QString fullimageUrl, QString previewimageUrl, QString video, bool read, QDateTime pts, int type, bool isMine, QString uid);
    void prependConversationElement(QString sender, QString senderId, QString timestamp, QString text, QString fullimageUrl, QString previewimageUrl, QString video, bool read, QDateTime pts, int type, QString uid);
    void deleteMsgWError(QString text);


private:
    QString getSenderName(QString chatId, QList<Participant> participants);
    QList<ConversationElement *> myList;

protected:
    QHash<int, QByteArray> roleNames() const;

signals:
    void conversationRequested(QString cid);

public slots:
    void conversationLoaded();

private slots:
    void deleteMsg(QNetworkReply *id);

};

#endif // CONVERSATIONMODEL_H
