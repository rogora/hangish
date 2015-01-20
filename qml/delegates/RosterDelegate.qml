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
            source: imagePath
        }

        Label {
            text: (unread > 0) ? qsTr(name + " " + unread) : qsTr(name)
            font.bold: (unread > 0) ? true : false
            anchors.verticalCenter: parent.verticalCenter
            color: delegate.highlighted ? Theme.highlightColor : Theme.primaryColor
        }
    }
    onClicked: {
        //var convPage = Qt.createComponent("Conversation.qml");
        console.log("Clicked " + id)
        conversation.loadConversationModel(id);
        rosterModel.readConv = id;
        pageStack.push(conversation);
    }
}
