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


#ifndef NOTIFIER_H
#define NOTIFIER_H

#include <QObject>
#include "contactsmodel.h"
#include "notification.h"

class Notifier : public QObject
{
    Q_OBJECT
public:
    Notifier(QObject *parent, ContactsModel *contacts);
    bool amITheActiveClient();
    bool isAnotherClientActive();

private:
    QList<Notification *> activeNotifications;
    int activeClientState;
    ContactsModel *cModel;
    QObject *myParent;
    uint lastId;

signals:
    void showNotificationForCover(int num);
    void deletedNotifications();
    void notificationPushed(QString convId);

public slots:
    void closeAllNotifications();
    void showNotification(QString preview, QString summary, QString body, QString sender, int num, QString convId);
    void activeClientUpdate(int state);

public Q_SLOTS:
    void test();
    void notificationPushedIntf(const QString &convId);

};

#endif // NOTIFIER_H
