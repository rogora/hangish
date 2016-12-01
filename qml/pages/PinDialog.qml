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


Dialog {
    id: dpage

    DialogHeader {
            id: dhead
            visible: true
            acceptText: qsTr("Login")
        }

    Connections
        {
            target: Client
            onInitFinished: {
                console.log("Init finished")
                pageStack.replace(Qt.resolvedUrl("Roster.qml"))
            }

            onAuthFailed: {
                loginIndicator.running = false
                infotext.text = error
                resultLabel.text = qsTr("Login Failed ") + error
                delauthbtn.visible = true
                pin.visible = true
            }
        }

    onAccepted: {
            Client.sendPin(pin.text.trim())
            loginIndicator.visible = true
            infotext.visible = true
            pin.visible = false
}

    Column {
        width: parent.width
        height: parent.height
        spacing: Theme.paddingLarge

        Item {
       // Spacer
       height: parent.height / 3
       width: 1
       }

    TextArea {
        id: pin
        label: "pin"
        placeholderText: "Pin"
        width: parent.width
        EnterKey.enabled: text.length > 0
        EnterKey.onClicked: dpage.accept()
    }

    BusyIndicator {
        visible: false
        id: loginIndicator
        running: true
        anchors.horizontalCenter: parent.horizontalCenter
        size: BusyIndicatorSize.Large
    }

    Label {
        id: infotext
        visible: false
        text: qsTr("Logging in")
        width: parent.width
        font {
            pixelSize: Theme.fontSizeLarge
            family: Theme.fontFamilyHeading
        }
        color: Theme.highlightColor
        horizontalAlignment: Text.AlignHCenter
    }

    Label {
        id: resultLabel
        color: "red"
    }

    Button {
        id: delauthbtn
        visible: false
        text: qsTr("Delete authentication cookie")
        onClicked: Client.deleteCookies()
        anchors.horizontalCenter: parent.horizontalCenter
        }
    }

}


