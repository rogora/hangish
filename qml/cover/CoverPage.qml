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


import QtQuick 2.0
import Sailfish.Silica 1.0

CoverBackground {
    //TODO: these 2 functions are hacky, need to figure out how to properly interact with the page stack
    //probably should use replaceAbove()
    function showRosterPage() {
            if (pageStack.depth === 2) {
                pageStack.pop()
            }
    }
    function showLastConvPage() {
        if (Client.getLastIncomingConversationId()==="") {
            showRosterPage();
            return;
        }
        //PageStack.update()
        conversation.loadConversationModel(Client.getLastIncomingConversationId());
        rosterModel.readConv = Client.getLastIncomingConversationId();
        conversation.conversationName = Client.getLastIncomingConversationName();
        if (pageStack.depth === 1) {
            pageStack.push(conversation);
        }
        else if (pageStack.depth === 3) {
            //we're in the picture selection page
            pageStack.pop();
        }
        conversation.openKeyboard();
    }

    property var unreadNum: 0
    Connections
        {
            target: Client
            onChannelLost: {
                coverIcon.rotation = 45
                coverIcon.source = "qrc:///icons/Hangish_ds.png"
            }
            onChannelRestored: {
                coverIcon.rotation = 0
                coverIcon.source = "qrc:///icons/Hangish.png"
            }
            onIsTyping: {
                if (status===1)
                    label.text = uname + qsTr(" is typing")
                else if (status===3)
                    label.text = ""
            }
            onShowNotificationForCover: {
                    console.log("caught shn from cover")
                    unreadNum = unreadNum + num
                    unreadlbl.text = unreadNum
            }
            onDeletedNotifications: {
                console.log("caught delete from cover")
                unreadNum = 0
                unreadlbl.text = ""
            }
        }

    Column {
        anchors.centerIn: parent
        width: parent.width

        Image {
                        id: coverIcon
                        source: "qrc:///icons/Hangish.png"
                        fillMode: Image.PreserveAspectFit
                        cache: true
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: parent.width/2
                        x: Theme.paddingSmall
                    }

        Label {
            id: label
            text: ""
            font.pixelSize: Theme.fontSizeExtraSmall
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Label {
            id: unreadlbl
            text: ""
            font.bold: unreadNum
            font.pixelSize: Theme.fontSizeMedium
            color: Theme.highlightColor
            anchors.horizontalCenter: parent.horizontalCenter

        }
    }
    CoverActionList {
        id: coverAction

        CoverAction {
            iconSource: "image://theme/icon-cover-subview"
            onTriggered: {
                showRosterPage()
                activate()
            }
        }

        CoverAction {
            iconSource: "image://theme/icon-cover-message"
            onTriggered: {
                showLastConvPage()
                activate()
            }
        }
    }
}


