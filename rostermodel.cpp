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


#include "rostermodel.h"

RosterModel::RosterModel(QObject *parent) :
    QAbstractListModel(parent)
{
}

QHash<int, QByteArray> RosterModel::roleNames() const {
    QHash<int, QByteArray> roles = QAbstractListModel::roleNames();
        roles.insert(ConvIdRole, QByteArray("id"));
        roles.insert(NameRole, QByteArray("name"));
        roles.insert(PartnumRole, QByteArray("particpantsNum"));
        roles.insert(UnreadRole, QByteArray("unread"));
        roles.insert(ImageRole, "imagePath");
        return roles;
}

int RosterModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return conversations.size();
}

QVariant RosterModel::data(const QModelIndex &index, int role) const
{
    //Reverse the order so that the most recent conv is displayed on top
    ConvAbstract * conv = conversations.at(conversations.size() - index.row() - 1);
    if (role == NameRole)
        return QVariant::fromValue(conv->name);
    else if (role == PartnumRole)
        return QVariant::fromValue(conv->participantsNum);
    else if (role == ConvIdRole)
        return QVariant::fromValue(conv->convId);
    else if (role == UnreadRole)
        return QVariant::fromValue(conv->unread);
    else if (role == ImageRole) {
        if (conv->imagePaths.size()) // TODO once we should support multiple images
            return conv->imagePaths;
        else
            return "";
    }

    return QVariant();
}

void RosterModel::setMySelf(User pmyself)
{
   myself = pmyself;
}


void RosterModel::addConversationAbstract(Conversation pConv)
{
    QString name = "";
    QStringList imagePaths;
    if (pConv.name.size() > 1) {
        name = pConv.name;
    } else {
        foreach (Participant p, pConv.participants) {
            if (p.user.chat_id!=myself.chat_id) {
                if (pConv.participants.last().user.chat_id != p.user.chat_id && pConv.participants.last().user.chat_id != myself.chat_id)  name += QString(p.user.display_name + ", ");
                else name += QString(p.user.display_name + " ");
                if (!p.user.photo.isEmpty()) {
                    QString image = p.user.photo;
                    if (!image.startsWith("https:")) image.prepend("https:");
                        imagePaths << image;
                }
                else {
                    imagePaths << "qrc:///icons/unknown.png";
                }
            }
        }
    }

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    conversations.append(new ConvAbstract(pConv.id, name, imagePaths, pConv.participants.size(), pConv.unread));
    endInsertRows();
}

bool RosterModel::conversationExists(QString convId)
{
    foreach (ConvAbstract *c, conversations) {
        if (c->convId == convId)
            return true;
    }

    return false;
}

void RosterModel::addUnreadMsg(QString convId)
{
    int i = 0;
    qDebug() << "adding unread msg " << convId;
    foreach (ConvAbstract *c, conversations) {
        if (c->convId == convId)
        {
            c->unread += 1;
            //put the referenced conversation on top of the list
            if (i!=conversations.length()-1) {
                conversations.move(i,conversations.length()-1);
            }
            break;
        }
        i++;
    }

    //TODO: check why this is the only (wrong) way to make it work!
    for (int i=0; i<conversations.size(); i++) {
        QModelIndex r1 = index(i);
        emit dataChanged(r1, r1);
    }
}

bool RosterModel::hasUnreadMessages(QString convId)
{
    foreach (ConvAbstract *c, conversations) {
        if (c->convId == convId)
        {
            return (c->unread > 0);
        }
     }
}

void RosterModel::putOnTop(QString convId)
{
    qDebug() << "Putting on top " << convId;
    int i = 0;
    foreach (ConvAbstract *c, conversations) {
        if (c->convId == convId)
        {
            //put the referenced conversation on top of the list
            if (i!=conversations.length()-1) {
                qDebug() << "Moving";
                conversations.move(i,conversations.length()-1);
                qDebug() << "Moved";
            }
            break;
        }
        i++;
    }

    //TODO: check why this is the only (wrong) way to make it work!
    for (int i=0; i<conversations.size(); i++) {
        QModelIndex r1 = index(i);
        emit dataChanged(r1, r1);
    }
}

void RosterModel::setReadConv(QString convId)
{
    int i = 0;
    qDebug() << "Setting as read " << convId;
    foreach (ConvAbstract *c, conversations) {
        if (c->convId == convId)
        {
            c->unread = 0;
            break;
        }
        i++;
    }

    //TODO: check why this is the only (wrong) way to make it work!
    for (int i=0; i<conversations.size(); i++) {
        QModelIndex r1 = index(i);
        emit dataChanged(r1, r1);
    }
}

QString RosterModel::getConversationName(QString convId) {
    foreach (ConvAbstract *ca, conversations) {
        if (ca->convId==convId)
            return ca->name;
    }
}
