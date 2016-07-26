import QtQuick 2.2
import Sailfish.Silica 1.0

    ListItem {
        id: delegate

        menu: ContextMenu {
            //enabled: fullimage.length>3
            MenuItem {
                text: qsTr("Open conversation")
                enabled: true
                onClicked: {
                    console.log(gaia_id)
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
                source: image.substr(0, 4) === "http" ? ImageHandler.getImageAddr(image) : image
            }

            Label {
                width: parent.width - img.width - 2 * Theme.paddingLarge
                text: display_name
                anchors.verticalCenter: parent.verticalCenter
                color: delegate.highlighted ? Theme.highlightColor : Theme.primaryColor
                maximumLineCount: 2
                wrapMode: Text.WordWrap
                truncationMode: TruncationMode.Fade
            }
        }

}
