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


Dialog {
    id: page
    DialogHeader {
        id: dhead
        visible: false
        acceptText: qsTr("Login")
    }

    Connections
        {
            target: Client
            onInitFinished: {
                column.visible = false
                dhead.visible = false
                loginIndicator.running = false
                infotext.text = "Logged in"
                infoColumn.visible = true
                pageStack.replace(Qt.resolvedUrl("Roster.qml"))
            }
            onLoginNeeded: {
                infoColumn.visible = false
                console.log("LOgin needed")
                dhead.visible = true
                column.visible = true
            }

            onAuthFailed: resultLabel.text = qsTr("Login Failed ") + error
        }


    onAccepted: {
        console.log("Accepted")
        if (pinField.text==="") {
            Client.sendCredentials(emailField.text.trim(), passwdField.text.trim())
        }
        else {
            console.log(pinField.text)
            Client.send2ndFactorPin(pinField.text)
        }
    }


    // To enable PullDownMenu, place our content in a SilicaFlickable
    /*
    SilicaFlickable {
        anchors.top: dhead.bottom

        // PullDownMenu and PushUpMenu must be declared in SilicaFlickable, SilicaListView or SilicaGridView
        PullDownMenu {
            MenuItem {
                text: qsTr("Conversations")
                onClicked: pageStack.push(Qt.resolvedUrl("Roster.qml"))
                //onClicked: pageStack.push(Qt.resolvedUrl("FullscreenImage.qml"))
            }
            MenuItem {
                text: qsTr("Log out")
                //onClicked: Client.testNotification()
                onClicked: Client.deleteCookies()
            }

        }


        // Tell SilicaFlickable the height of its content.
        contentHeight: column.height
        */

        // Place our content in a Column.  The PageHeader is always placed at the top
        // of the page, followed by our content.
        Column {

            id: column
            visible: false
            anchors.top: dhead.bottom

            width: page.width
            spacing: Theme.paddingLarge

            TextField {
                id: emailField
                placeholderText: "Email"
                label: "Email"
                width: parent.width
            }
            TextField {
                id: passwdField
                echoMode: TextInput.Password
                placeholderText: "Password"
                label: "Password"
                width: parent.width
            }
            TextField {
                id: pinField
                //readOnly: true
                placeholderText: "Leave empty if not needed"
                label: "2nd factor pin"
                width: parent.width
            }
            Button {
                id: sendButton
                width: 140
                text: "Send pin"
                onClicked: {
                    Client.send2ndFactorPin(pinField.text)
                    resultLabel.text = "Logging in... wait"
                }
            }
            Label {
                id: resultLabel
                color: "red"
            }
        }
        Column {
            anchors.fill: parent
            id: infoColumn
            visible: true
            width: parent.width
            height: parent.height

            spacing: Theme.paddingLarge

            Item {
                // Spacer
                height: parent.height / 3
                width: 1
            }

            BusyIndicator {
                id: loginIndicator
                running: true
                anchors.horizontalCenter: parent.horizontalCenter
                size: BusyIndicatorSize.Large
            }

            Label {
                id: infotext
                text: qsTr("Logging in")
                width: parent.width
                font {
                    pixelSize: Theme.fontSizeLarge
                    family: Theme.fontFamilyHeading
                }
                color: Theme.highlightColor
                horizontalAlignment: Text.AlignHCenter
            }
        }
   // }
}


