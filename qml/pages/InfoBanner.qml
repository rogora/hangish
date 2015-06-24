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

DockedPanel {
    id: root
    visible: false

    width: Screen.width - 2*Theme.paddingSmall
    height: content.height + 2*Theme.paddingSmall

    dock: Dock.Bottom

    Rectangle {
        id: content
        visible: false
        x: Theme.paddingSmall
        y: Theme.paddingSmall
        width: parent.width

        height: infoLabel.height + 2*Theme.paddingSmall
        color: 'black';
        opacity: 0.65;

        Label {
            id: infoLabel
            text : ''
            color: Theme.highlightColor
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMedium
            width: parent.width - 2*Theme.paddingSmall
            x: Theme.paddingSmall
            y: Theme.paddingSmall
            wrapMode: Text.WrapAnywhere
            }
        MouseArea {
            anchors.fill: parent
            onClicked: {
                root.visible = false
                content.visible = false
                root.hide()
                autoClose.stop()
            }
        }
    }


    function displayError(errorMsg) {
        root.visible = true
        content.visible = true
        infoLabel.text = errorMsg
        root.show()
        autoClose.start()
    }

    Timer {
        id: autoClose
        interval: 3000
        running: false
        onTriggered: {
            root.visible = false
            content.visible = false
            root.hide()
            stop()
        }

    }
}
