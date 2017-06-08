import QtQuick 2.1
import Sailfish.Silica 1.0

Dialog
{

    id: settingspage
    allowedOrientations: defaultAllowedOrientations
    acceptDestinationAction: PageStackAction.Pop

    onAccepted: {
        Client.toggleDaemonize(tsdaemonenabled.checked);
    }

    SilicaFlickable
    {
        anchors.fill: parent
        contentHeight: content.height

        Column
        {
            id: content
            width: parent.width

            DialogHeader { acceptText: qsTr("Save") }

            TextSwitch
            {
                id: tsdaemonenabled
                anchors { left: parent.left; right: parent.right; leftMargin: Theme.paddingSmall; rightMargin: Theme.paddingSmall }
                text: qsTr("Background")
                description: qsTr("Hangish will continue working in background after closing")
                checked: Client.getDaemonize()
            }
        }
    }
}
