import QtQuick 2.0
import Sailfish.Silica 1.0

BackgroundItem {
    id: delegate
    height: itemId.height + 10

    property string styleSheets: "<style type='text/css'>a:link {color:%1}</style>".arg(Theme.highlightColor)

    Item {
        id: itemId
        width: parent.width
        height: msgTextLabel.paintedHeight + senderLabel.paintedHeight + imageView.paintedHeight
        Column {
            width: parent.width * 0.75
            anchors.left: senderId == page.selfChatId ? parent.left : undefined
            anchors.right: senderId != page.selfChatId ? parent.right : undefined
            anchors.leftMargin: Theme.paddingLarge
            anchors.rightMargin: Theme.paddingLarge


            Label {
                id: msgTextLabel
                width: parent.width
                text: styleSheets + msgtext
                wrapMode: Text.Wrap
                elide: Text.ElideRight
                truncationMode: TruncationMode.Fade
                horizontalAlignment: (senderId == page.selfChatId) ? Text.AlignLeft : Text.AlignRight
                color: (senderId == page.selfChatId) ? Theme.highlightColor : Theme.primaryColor
                textFormat: Text.RichText
                onLinkActivated: Qt.openUrlExternally(link)
            }

            Image {
                id: imageView
                source: previewimage
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
            Image {
                id: hasReadImage

            }

        }
    }
    onClicked: openFullImage(fullimage)

    function openFullImage(url) {
        console.log(url)
        if (url.length>3) {
            fsImage.loadImage(url)
            pageStack.push(fsImage)
        }
    }
}

