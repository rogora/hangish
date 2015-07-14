import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
            function setConvId(cid) {
                convId = cid
            }

            id: convRenameInput

            property string convId

            property alias initialValue: inputField.text
            property alias value: inputField.text

            property alias title: inputField.label
            property alias inputMethodHints: inputField.inputMethodHints

            Component.onCompleted: inputField.forceActiveFocus()

            DialogHeader {
                id: header
            }

            TextField {
                id: inputField
                anchors.top: header.bottom
                width: parent.width

                placeholderText: label

                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: parent.accept()
            }

            onAccepted: {
                roster.renameConv(convId, value)
            }

}
