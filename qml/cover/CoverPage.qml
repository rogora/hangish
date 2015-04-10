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
    property var unreadNum: 0
    Connections
        {
            target: Client
            onChannelLost: {
                coverIcon.rotation = 45
            }
            onChannelRestored: {
                coverIcon.rotation = 0
            }
            onIsTyping: {
                if (status===1)
                    label.text = qsTr(uname + " is typing")
                else if (status===3)
                    label.text = qsTr("Hangouts online")
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
                        source: "/usr/share/icons/hicolor/86x86/apps/Hangish.png"
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
        }

        Label {
            id: unreadlbl
            text: ""
            font.bold: unreadNum
            font.pixelSize: Theme.fontSizeMedium
            color: Theme.highlightColor
        }
    }
    /*
    CoverActionList {
        id: coverAction

        CoverAction {
            iconSource: "image://theme/icon-cover-next"
            onTriggered: Client.forceChannelRestore()
        }

        CoverAction {
            iconSource: "image://theme/icon-cover-pause"
        }
    }
        */

}


