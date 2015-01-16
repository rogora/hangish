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


#include <sailfishapp.h>
#include <client.h>
#include "conversationmodel.h"
#include "rostermodel.h"
#include "contactsmodel.h"
#include "filemodel.h"


int main(int argc, char *argv[])
{
    QGuiApplication *app = SailfishApp::application(argc, argv);
    QQuickView *view = SailfishApp::createView();
    //view->rootContext()->setContextProperty("cppproperty", view);
    ConversationModel *conversationModel = new ConversationModel();
    RosterModel *rosterModel = new RosterModel();
    ContactsModel *contactsModel = new ContactsModel();
    FileModel *fileModel = new FileModel();
    Client *c = new Client(view, rosterModel, conversationModel, contactsModel);

    //qmlRegisterType<ConversationModel>("org.rogora.hangouts", 1, 0, "ConversationModel");
    view->rootContext()->setContextProperty("conversationModel", conversationModel);
    view->rootContext()->setContextProperty("rosterModel", rosterModel);
    view->rootContext()->setContextProperty("contactsModel", contactsModel);
    view->rootContext()->setContextProperty("fileModel", fileModel);
    view->engine()->rootContext()->setContextProperty("Client", c);
//    view->rootContext()->setContextProperty("Client", c);

    QDBusConnection system = QDBusConnection::systemBus();
        if (!system.isConnected())
        {
           qFatal("Cannot connect to the D-Bus session bus.");
            return 1;
         }
        system.connect("net.connman",
                          "/net/connman/technology/wifi",
                          "net.connman.Technology",
                          "PropertyChanged",
                          c,
                          SLOT(connectivityChanged(QString,QDBusVariant))
                          );
        system.connect("net.connman",
                          "/net/connman/technology/cellular",
                          "net.connman.Technology",
                          "PropertyChanged",
                          c,
                          SLOT(connectivityChanged(QString,QDBusVariant))
                          );

    view->setSource(SailfishApp::pathTo("qml/Hangish.qml"));
    view->showFullScreen();
    app->exec();
    //return SailfishApp::main(argc, argv);
}

