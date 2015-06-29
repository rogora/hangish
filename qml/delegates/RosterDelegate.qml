import QtQuick 2.0
import Sailfish.Silica 1.0

BackgroundItem {
    id: delegate

    height: img.height

    Row {
        height: parent.height
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
            source: imagePath.substr(0, 4) === "http" ? ImageHandler.getImageAddr(imagePath) : imagePath
            Connections
                {
                    id: roster_conn
                    target: ImageHandler
                    onImgReady: {
                        //if (path == imagePath) {
                            console.log("Updating!")
                            img.source = imagePath.substr(0, 4) === "http" ? ImageHandler.getImageAddr(imagePath) : imagePath
                        //}
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
