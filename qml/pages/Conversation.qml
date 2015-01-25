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

    property string conversationId: ""
    property string conversationName: ""
    property string selfChatId;

    InfoBanner {
        id: ibanner
    }
    //property int pIndex

    /*
    onStatusChanged: {
        console.log(page.status)
        console.log("CONV Entered")
        console.log(conversationId)
    }
    */


    function loadConversationModel(cid) {
        Client.updateWatermark(cid)
        Client.setFocus(cid, 1)
        page.selfChatId = Client.getSelfChatId()
        page.conversationId = cid;
        conversationModel.cid = cid;
        listView.positionViewAtEnd()
    }

    SilicaListView {
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
                TextArea {
                    objectName: "sendTextArea"
                    id: sendBox
                    width: 400
                    focus: true
                    color: Theme.highlightColor
                    font.family: "cursive"
                    placeholderText: qsTr("Reply")
                    property int typingStatus: 3
                    onTextChanged: {
                        console.log("Text changed!")
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
                            console.log(qsTr("Empty message"))
                            ibanner.displayError(qsTr("Can't send empty message"))
                            return
                        }

                        Client.sendChatMessage(sendBox.text.trim(), page.conversationId);
                        sendBox.text = "";
                        sendBox.placeholderText = qsTr("Sending message...");
                        sendBox.text = "";
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
                        sendBox.placeholderText = "Offline"
                    }
                    function setOnline() {
                        sendButton.enabled = true
                        sendBox.placeholderText = "Reply"
                    }
                    function setSendError() {
                        ibanner.displayError("Error sending msg")
                        sendBox.placeholderText = "Reply"
                    }
                    function setIsTyping(id, convId, status) {
                        console.log("is typing")
                        console.log(id)
                        console.log(convId)
                        if (page.conversationId === convId) {
                            if (status === 1)
                                sendBox.placeholderText = qsTr(id + " is typing")
                            else if (status === 2)
                                sendBox.placeholderText = qsTr(id + " paused")
                            else if (status === 3)
                                sendBox.placeholderText = qsTr("Reply")
                        }
                    }


                }
                Button {
                    id: sendButton
                    width: 140
                    text: "Send"
                    onClicked: sendBox.sendMessage()
                    onPressAndHold: {
                        fileModel.searchPath = "/home/nemo/"
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
                    }
            }
    }

}
