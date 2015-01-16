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


#include "contactsmodel.h"

ContactsModel::ContactsModel(QObject *parent) :
    QAbstractListModel(parent)
{
}

void ContactsModel::addContact(User pContact)
{
    contacts.append(pContact);
}

QString ContactsModel::getContactFName(QString cid)
{
    foreach (User u, contacts)
        if (u.chat_id == cid)
            return u.first_name;
    return "Unknown";
}

QString ContactsModel::getContactDName(QString cid)
{
    foreach (User u, contacts)
        if (u.chat_id == cid)
            return u.display_name;
    return cid;
}

int ContactsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return contacts.size();
}

QVariant ContactsModel::data(const QModelIndex &index, int role) const
{

    return QVariant();
}
