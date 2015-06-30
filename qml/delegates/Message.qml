import QtQuick 2.0
import Sailfish.Silica 1.0

BackgroundItem {
    id: delegate
    height: itemId.height + 10
    ListItem {
        id: itemId
        menu: ContextMenu {
            //enabled: fullimage.length>3
            MenuItem {
                text: qsTr("Download to gallery")
                enabled: fullimage.length>3
                onClicked: ImageHandler.saveImageToGallery(fullimage)
            }
        }
        width: parent.width
        contentHeight: msgTextLabel.paintedHeight + senderLabel.paintedHeight + imageView.paintedHeight
        //height: msgTextLabel.paintedHeight + senderLabel.paintedHeight + imageView.paintedHeight
        Column {
            width: parent.width * 0.75
            anchors.left: senderId == page.selfChatId ? parent.left : undefined
            anchors.right: senderId != page.selfChatId ? parent.right : undefined
            anchors.leftMargin: Theme.paddingLarge
            anchors.rightMargin: Theme.paddingLarge

            Label {
                id: msgTextLabel
                width: parent.width
                text: msgtext
                wrapMode: Text.Wrap
                elide: Text.ElideRight
                truncationMode: TruncationMode.Fade
                horizontalAlignment: (senderId == page.selfChatId) ? Text.AlignLeft : Text.AlignRight
                color: (senderId == page.selfChatId) ? Theme.highlightColor : Theme.primaryColor
                textFormat: Text.StyledText
                onLinkActivated: Qt.openUrlExternally(link)
            }

            AnimatedImage {
                id: imageView
                source: previewimage != "" ? ImageHandler.getImageAddr(previewimage) : previewimage
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                cache: true
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width / 6 * 5
                x: Theme.paddingSmall
            }

           Label {
                id: senderLabel
                width: parent.width
                font.pixelSize: Theme.fontSizeExtraSmall
                font.bold: !read
                font.italic: !read
                text: (sender != "" ? sender + ", " : "") + timestamp
                horizontalAlignment: (senderId == page.selfChatId) ? Text.AlignLeft : Text.AlignRight
                color: (senderId == page.selfChatId) ? Theme.secondaryHighlightColor : Theme.secondaryColor
            }

        }

        Connections
            {
                target: ImageHandler
                onImgReady: {
                    imageView.source = previewimage != "" ? ImageHandler.getImageAddr(previewimage) : previewimage
                }
            }

        onClicked: {
            if (timestamp != "Error, msg not sent!" && fullimage.length>3) {
                openFullImage(fullimage)
            }
            else if (timestamp == "Error, msg not sent!") {
                conversation.openText(msgtext)
                Client.deleteMessageWError(msgtext);
            }
        }
    }



    function openFullImage(url) {
        console.log(url)
        if (url.length>3) {
            fsImage.loadImage(url)
            pageStack.push(fsImage)
        }
    }
}

