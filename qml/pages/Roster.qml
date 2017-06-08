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


import QtQuick 2.2
import Sailfish.Silica 1.0

import "../delegates"

Page {
    id: page
    objectName: "roster"
    allowedOrientations: Orientation.All

    function renameConv(cid, newName) {
        console.log(newName)
        Client.renameConversation(cid, newName)
    }

    onStatusChanged: {
        console.log(page.status)
        if (page.status==2) {
            Client.setFocus(conversationModel.cid, 2)
            console.log("Resetting conv")
            conversationModel.cid = ""
        }
    }

    RemorsePopup {
        id: remorse
    }

    SilicaListView {
        PullDownMenu {
            MenuItem {
                text: qsTr("About Hangish")
                onClicked: pageStack.push(about) //Client.testNotification();
            }
            MenuItem {
                text: qsTr("Log out")
                onClicked: remorse.execute(qsTr("Deleting cookies"),
                                           function()
                                           {
                                               Client.deleteCookies();
                                           } );
            }
            /*
            MenuItem {
                text: qsTr("Kill Hangish")
                onClicked: Client.kill(); //Client.testNotification();
            }
            */
            MenuItem {
                text: qsTr("Options")
                onClicked: pageStack.push(options) //Client.testNotification();
            }
            /*
            MenuItem {
                text: qsTr("Contacts")
                onClicked: pageStack.push(contacts) //Client.testFunction()
            }
            */
        }


        id: listView
        model: rosterModel
        anchors.fill: parent
        header: PageHeader {
            title: qsTr("Conversations")
        }
        delegate: RosterDelegate {
            onClicked: {
                console.log("Clicked " + id)
                conversation.loadConversationModel(id);
                rosterModel.readConv = id;
                pageStack.push(conversation);
            }
        }
        VerticalScrollDecorator {}
    }
    Connections {
        target: Client
        onNotificationPushed: {
            console.log("Clicked " + convId)
            if (convId!="foo") {
                conversation.loadConversationModel(convId);
                rosterModel.readConv = convId;
                if (pageStack.depth==1)
                    pageStack.push(conversation);
            }
            activate()
        }
    }

    Connections {
        target: listView.model
        onDataChanged: {
            //console.log("Changing data")
        }
    }

    Connections {
        target: Qt.application
        onActiveChanged: {
                if(Qt.application.state !== Qt.ApplicationActive) {
                    // Pauze the game here
                    console.log("app paused")
                    if (conversationModel.cid != "")
                        Client.setFocus(conversationModel.cid, 2)
                    Client.setAppPaused()
                }
                else {
                    console.log("app opened")
                    if (conversationModel.cid != "")
                        Client.setFocus(conversationModel.cid, 1)
                    Client.setAppOpened()
                }
            }
        }
}



