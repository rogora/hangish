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


#ifndef CONTACTSMODEL_H
#define CONTACTSMODEL_H

#include <QAbstractListModel>
#include "qtimports.h"
#include "structs.h"

class ContactsModel : public QAbstractListModel
{
    QList<User> contacts;
    User myself;

    Q_OBJECT
public:
    enum ContactRoles {
        NameRole = Qt::UserRole + 1,
        FirstNameRole,
        GaiaIdRole,
        ChatIdRole,
        ImageRole,
        EmailRole
    };

    explicit ContactsModel(QObject *parent = 0);
    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    void addContact(User pContact);

    Q_INVOKABLE QString getContactFName(QString cid);
    Q_INVOKABLE QString getContactDName(QString cid);
    Q_INVOKABLE void searchPhoneContacts();


signals:

public slots:

protected:
    QHash<int, QByteArray> roleNames() const;

};

#endif // CONTACTSMODEL_H
