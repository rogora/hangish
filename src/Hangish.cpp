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


#include <QtCore/QScopedPointer>

#include <sailfishapp.h>
#include <client.h>
#include "imagehandler.h"
#include "conversationmodel.h"
#include "rostermodel.h"
#include "contactsmodel.h"
#include "filemodel.h"


int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("Hangish");
    QCoreApplication::setApplicationName("Hangish");
    QCoreApplication::setApplicationVersion("0.9.0");

    qsrand(QTime::currentTime().msec());

    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));

    // Many thanks to sailorgram for daemon handling
    QDBusConnection sessionbus = QDBusConnection::sessionBus();

        if(sessionbus.interface()->isServiceRegistered("harbour.hangish")) // Only a Single Instance is allowed
        {
            qDebug() << "opened already";
            QDBusMessage message = QDBusMessage::createMethodCall("harbour.hangish", "/", "harbour.hangish", "wakeUp");
            QDBusConnection connection = QDBusConnection::sessionBus();
            connection.send(message);
            if(app->hasPendingEvents())
                app->processEvents();

            return 0;
        }
        else {
            qDebug() << "first instance";
        }

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    ConversationModel *conversationModel = new ConversationModel();
    RosterModel *rosterModel = new RosterModel();
    ContactsModel *contactsModel = new ContactsModel();
    FileModel *fileModel = new FileModel();
    Client *c = new Client(rosterModel, conversationModel, contactsModel);
    app->setQuitOnLastWindowClosed(!c->getDaemonize());
    ImageHandler *ih = new ImageHandler();
    ih->setAuthenticator(c->getAuthenticator());
    //Do this once when app is launching
    ih->cleanCache();


    QTranslator translator;
        translator.load("translations_" + QLocale::system().name(),
                        "/usr/share/harbour-hangish/translations");
        app->installTranslator(&translator);

    view->rootContext()->setContextProperty("conversationModel", conversationModel);
    view->rootContext()->setContextProperty("rosterModel", rosterModel);
    view->rootContext()->setContextProperty("contactsModel", contactsModel);
    view->rootContext()->setContextProperty("fileModel", fileModel);
    view->engine()->rootContext()->setContextProperty("Client", c);
    view->engine()->rootContext()->setContextProperty("ImageHandler", ih);


    QDBusConnection system = QDBusConnection::systemBus();
        if (!system.isConnected())
        {
           qFatal("Cannot connect to the D-Bus session bus.");
            return 1;
         }
        system.connect("net.connman",
                          "/",
                          "net.connman.Manager",
                          "PropertyChanged",
                          c,
                          SLOT(connectivityChanged(QString,QDBusVariant))
                          );

    view->setSource(SailfishApp::pathTo("qml/harbour-hangish.qml"));
    view->showFullScreen();
    app->exec();
}

