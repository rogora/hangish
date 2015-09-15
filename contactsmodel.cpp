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

QHash<int, QByteArray> ContactsModel::roleNames() const {
    QHash<int, QByteArray> roles = QAbstractListModel::roleNames();
        roles.insert(NameRole, QByteArray("display_name"));
        roles.insert(FirstNameRole, QByteArray("first_name"));
        roles.insert(GaiaIdRole, QByteArray("gaia_id"));
        roles.insert(ChatIdRole, "chat_id");
        roles.insert(ImageRole, "image");
        roles.insert(EmailRole, "email");
        return roles;
}

void ContactsModel::searchPhoneContacts()
{
    QtContacts::QContactManager *manager = new QtContacts::QContactManager();
    QList<QtContacts::QContact> results = manager->contacts();
    qDebug() << results.size();
    foreach (QtContacts::QContact c, results) {
        QtContacts::QContactPhoneNumber no = c.detail<QtContacts::QContactPhoneNumber>();
        QtContacts::QContactName name = c.detail<QtContacts::QContactName>();
        qDebug() << name.firstName() << " " << name.lastName() << ": " << no.number();
/*        foreach (QtContacts::QContactDetail d, details) {
            qDebug() << "New detail";
            qDebug() << d.value(QtContacts::QContactDetail::TypeName).toString();
        }
        */
    }
}

void ContactsModel::addContact(User pContact)
{
    if (!pContact.photo.startsWith("https:")) pContact.photo.prepend("https:");

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    contacts.append(pContact);
    endInsertRows();
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
    User contact = contacts.at(contacts.size() - index.row() - 1);
    if (role == NameRole)
        return QVariant::fromValue(contact.display_name);
    else if (role == FirstNameRole)
        return QVariant::fromValue(contact.first_name);
    else if (role == GaiaIdRole)
        return QVariant::fromValue(contact.gaia_id);
    else if (role == ChatIdRole)
        return QVariant::fromValue(contact.chat_id);
    else if (role == ImageRole) {
        if (contact.photo.size()) { // TODO once we should support multiple images
            return contact.photo;
        }
        else {
            return "";
        }
    }

    else if (role == EmailRole) {
            return QVariant::fromValue(contact.email);
    }

    return QVariant();
}
