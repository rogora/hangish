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


#include "notifier.h"

Notifier::Notifier(QObject *parent, ContactsModel *contacts) :
    QObject(parent)
{
    cModel = contacts;
    myParent = parent;
    lastId = 0;
}

void Notifier::showNotification(QString preview, QString summary, QString body, QString sender)
{
    qDebug() << "shn called";
    //Check notification aspect
    Notification *n = new Notification(myParent);

    /*
     * Temporarily disabled; how does it work exactly?
    if (lastId > 0)
        n->setReplacesId(lastId);
    */
    n->setCategory("im.received");
    n->setHintValue("x-nemo-feedback", "chat");
    n->setHintValue("x-nemo-priority", 100);
    n->setHintValue("x-nemo-preview-icon", "icon-s-status-chat");
    n->setHintValue("lowPowerModeIconId", "icon-m-low-power-mode-chat");
    n->setHintValue("statusAreaIconId", "icon-s-status-notifier-chat");
    n->setItemCount(1);
    n->setBody(body);
    n->setPreviewBody(preview);

    n->setSummary(cModel->getContactDName(sender));

    n->setTimestamp(QDateTime::currentDateTime());
    /*
    n->setRemoteDBusCallServiceName("hangouts");
    n->setRemoteDBusCallObjectPath("/example");
    n->setRemoteDBusCallInterface("org.nemomobile.example");
    n->setRemoteDBusCallMethodName("doSomething");
    */
    n->publish();
    lastId = n->replacesId();
    qDebug() << "pubbed " << n->replacesId();
}
