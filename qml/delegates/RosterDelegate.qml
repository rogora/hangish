import QtQuick 2.0
import Sailfish.Silica 1.0

    ListItem {
        id: delegate
        property int imageRotation: 0

        menu: ContextMenu {
            //enabled: fullimage.length>3
            MenuItem {
                text: qsTr("Rename")
                //enabled: fullimage.length>3
                onClicked: {
                    convRenameInput.setConvId(id)
                    pageStack.push(convRenameInput)
                }
            }
            MenuItem {
                text: qsTr("Archive/Leave")
                //enabled: fullimage.length>3
                onClicked: remorse.execute(qsTr("Leaving conversation"),
                                           function()
                                           {
                                               console.log("Should leave conv here")
                                               console.log(participantsNum)
                                               if (participantsNum > 2) {
                                                   Client.leaveConversation(id)
                                               }
                                               else {
                                                    Client.deleteConversation(id)
                                               }
                                           } );
            }
            MenuItem {
                text: notifLevel == 10 ? qsTr("Reenable notifications") : qsTr("Silence")
                //enabled: fullimage.length>3
                onClicked: {
                    console.log(notifLevel)
                }
            }
        }

        contentHeight: img.paintedHeight
        //height: img.height

        Row {
            height: img.paintedHeight
            anchors {
                left: parent.left
                right: parent.right
                rightMargin: Theme.paddingLarge
            }
            spacing: Theme.paddingMedium

            Image {
                id: img
                width: Theme.iconSizeLarge
                height: width
                asynchronous: true
                cache: true
                source: imagePath[imageRotation % imagePath.length].substr(0, 4) === "http" ? ImageHandler.getImageAddr(imagePath[imageRotation% imagePath.length]) : imagePath[imageRotation% imagePath.length]

                states: [
                    State {
                        name: "fade";
                        PropertyChanges { target: img; opacity: 0.1}
                    },
                    State {
                        name: "show";
                        PropertyChanges { target: img; opacity: 1}
                    }
                ]
                transitions: Transition {
                        NumberAnimation { properties: "opacity"; easing.type: Easing.InOutQuad; duration: 500  }
                    }
                Connections
                    {
                        id: roster_conn
                        target: ImageHandler
                        onImgReady: {
                            if (path == imagePath) {
                                console.log("Updating!")
                                img.source = imagePath[imageRotation].substr(0, 4) === "http" ? ImageHandler.getImageAddr(imagePath[imageRotation]) : imagePath[imageRotation]
                            }
                        }
                    }
                Connections {
                    target: Qt.application
                    onActiveChanged: {
                        if(!Qt.application.active) {
                            // Pauze the game here
                            timer.stop()
                        }
                        else {
                            timer.restart()
                        }
                    }
                }

                Timer {
                    id: timer
                    interval: 10000
                    repeat: true
                    running: true

                    onTriggered:
                    {
                        if (imagePath.length > 1) {
                            img.state = "fade"
                            timer2.start()
                        }
                    }
                }

                Timer {
                    id: timer2
                    interval: 500
                    repeat: false
                    running: false

                    onTriggered:
                    {
                        if (imagePath.length > 1) {
                            imageRotation = (imageRotation + 1) % imagePath.length
                            //img.source = imagePath[imageRotation].substr(0, 4) === "http" ? ImageHandler.getImageAddr(imagePath[imageRotation]) : imagePath[imageRotation]
                            img.state = "show"
                        }
                    }
                }
            }

            Label {
                width: parent.width - img.width - 2 * Theme.paddingLarge
                text: (unread > 0) ? qsTr(name + " " + unread) : qsTr(name)
                font.bold: (unread > 0) ? true : false
                anchors.verticalCenter: parent.verticalCenter
                color: delegate.highlighted ? Theme.highlightColor : Theme.primaryColor
                maximumLineCount: 2
                wrapMode: Text.WordWrap
                truncationMode: TruncationMode.Fade
            }
        }

}
