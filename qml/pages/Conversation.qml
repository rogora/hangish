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

import "../delegates"

Page {
    id: page
    objectName: "conversation"
    allowedOrientations: Orientation.All

    property string conversationId: ""
    property string conversationName: ""
    property string selfChatId;

    InfoBanner {
        id: ibanner
    }

    function loadConversationModel(cid) {
        Client.updateWatermark(cid)
        Client.setFocus(cid, 1)
        page.conversationName = Client.getConversationName(cid)
        page.selfChatId = Client.getSelfChatId()
        page.conversationId = cid;
        conversationModel.cid = cid;
        listView.positionViewAtEnd()
    }

    function openText(text) {
        listView.footerItem.openText(text)
    }

    function openKeyboard() {
        listView.footerItem.openKeyboard()
    }

        SilicaListView {
            PullDownMenu {
                MenuItem {
                    text: qsTr("Load more...")
                    onClicked: Client.retrieveConversationLog(page.conversationId)
                    }
            }

            id: listView
            model: conversationModel
            anchors.fill: parent
            width: parent.width
            header: PageHeader {
                title: page.conversationName
            }
            delegate: Message { }
            VerticalScrollDecorator {}
            footer: Row {
                    width: page.width
                    TextArea {
                        objectName: "sendTextArea"
                        id: sendBox
                        anchors.bottom: parent.bottom
                        width: parent.width - sendButton.width
                        focus: true
                        color: Theme.highlightColor
                        font.family: "cursive"
                        placeholderText: qsTr("Reply")

                        property int typingStatus: 3

                        EnterKey.enabled: true
                        EnterKey.highlighted: true
                        EnterKey.iconSource: "image://theme/icon-m-enter-next"
                        EnterKey.onClicked: {
                            sendMessage()
                            sendBox.select()
                        }

                        onTextChanged: {
                            //console.log("Text changed!")
                            if (sendBox.text!=="" && typingStatus === 3) {
                                typingStatus = 1
                                Client.setTyping(page.conversationId, typingStatus)
                            }
                            if (sendBox.text==="" && typingStatus !== 3) {
                                typingStatus = 3
                                Client.setTyping(page.conversationId, typingStatus)
                            }
                        }

                        function sendMessage() {
                            if (sendBox.text.trim()==="") {
                                console.log("Empty message")
                                ibanner.displayError(qsTr("Can't send empty message"))
                                sendBox.text = "";
                                return
                            }

                            Client.sendChatMessage(sendBox.text.replace(/(\n)/gm,""), page.conversationId);
                            sendBox.deselect();
                            sendBox.text = "";
                            sendBox.placeholderText = qsTr("Sending message...");
                        }
                        function sendImage(path) {
                            sendButton.enabled = false
                            Client.sendImage("foo", page.conversationId, path)
                            sendBox.placeholderText = qsTr("Sending image...");
                            sendBox.text = "";
                            imagepicker.selected.disconnect(sendBox.sendImage)
                        }
                        function setOffline() {
                            sendButton.enabled = false
                            sendBox.placeholderText = qsTr("Offline")
                        }
                        function setOnline() {
                            sendButton.enabled = true
                            sendBox.placeholderText = qsTr("Reply")
                        }
                        function setSendError() {
                            ibanner.displayError(qsTr("Error sending msg"))
                            sendBox.placeholderText = qsTr("Reply")
                        }
                        function setUploadError() {
                            ibanner.displayError(qsTr("Error uploading image"))
                            sendBox.placeholderText = qsTr("Reply")
                        }

                        function setIsTyping(id, convId, status) {
                            console.log("is typing")
                            console.log(id)
                            console.log(convId)
                            if (page.conversationId === convId) {
                                if (status === 1)
                                    sendBox.placeholderText = id + qsTr(" is typing")
                                else if (status === 2)
                                    sendBox.placeholderText = id + qsTr(" paused")
                                else if (status === 3)
                                    sendBox.placeholderText = qsTr("Reply")
                            }
                        }
                    }
                    IconButton {
                        id: sendButton
                        width: 140
                        //This is commented out because:
                        // - it allows (as desired) the button to stick to the bottom
                        // - BUT the button is drawn too low, slightly below the sendBox, making the page ugly

                        //anchors.top: parent.bottom
                        //text: "send"
                        icon.source: "image://theme/icon-l-right"
                        onClicked: sendBox.sendMessage()
                        onPressAndHold: {
                            //Workaround for rpm validator
                            fileModel.searchPath = "foo"
                            pageStack.push(imagepicker)
                            imagepicker.selected.connect(sendBox.sendImage)
                        }
                     }
                    Connections
                        {
                            target: Client
                            onChannelLost: sendBox.setOffline()
                            onChannelRestored: sendBox.setOnline()
                            onIsTyping: sendBox.setIsTyping(uname, convid, status)
                            onMessageSent: sendBox.setOnline()
                            onMessageNotSent: sendBox.setSendError()
                            onImageUploadFailed: sendBox.setUploadError()
                        }

                    Connections
                        {
                            target: ImageHandler
                            onSavedToGallery: ibanner.displayError(qsTr("Image saved to gallery!"))                        }
                    function openKeyboard() {
                        sendBox.forceActiveFocus()
                    }
                    function openText(text) {
                        sendBox.text = text;
                    }

                }
          /*
            PushUpMenu {
                    Text {
                        color: Theme.highlightColor
                        font.family: "cursive"
                        text: "Hello, Sailor!"
                    }
            }
        }
        */
    }

}
