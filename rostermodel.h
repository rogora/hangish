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


#ifndef ROSTERMODEL_H
#define ROSTERMODEL_H

#include <QAbstractListModel>
#include "structs.h"

class ConvAbstract {
public:
    QString convId;
    QString name;
    QStringList imagePaths;
    int participantsNum;
    int unread;

    ConvAbstract(QString pconvId, QString pname, QStringList imgPaths, int ppNum, int pUnread) {
        convId = pconvId;
        name = pname;
        imagePaths = imgPaths;
        participantsNum = ppNum;
        unread = pUnread;
    }
};

class RosterModel : public QAbstractListModel
{
    User myself;

    Q_OBJECT

public:
    void setMySelf(User pmyself);
    enum ConversationRoles {
        NameRole = Qt::UserRole + 1,
        PartnumRole,
        ConvIdRole,
        UnreadRole,
        ImageRole
    };
    explicit RosterModel(QObject *parent = 0);
    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    void addConversationAbstract(Conversation pConv);
    bool conversationExists(QString convId);
    void setReadConv(QString convId);
    void addUnreadMsg(QString convId);
    bool hasUnreadMessages(QString convId);
    QString getConversationName(QString convId);

private:
    QList<ConvAbstract *> conversations;

signals:

public slots:

protected:
    QHash<int, QByteArray> roleNames() const;

};

#endif // ROSTERMODEL_H
