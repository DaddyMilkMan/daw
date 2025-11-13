/**
 * Sidebar Panel - Browser and devices
 */

import QtQuick
import QtQuick.Controls
import "../styles"

Rectangle {
    id: sidebar
    color: theme.panelColor

    AppTheme { id: theme }

    Column {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Label {
            text: "BROWSER"
            font.pixelSize: 16
            font.bold: true
            color: theme.textColor
        }

        TabBar {
            id: tabBar
            width: parent.width

            TabButton {
                text: "Files"
            }
            TabButton {
                text: "Plugins"
            }
            TabButton {
                text: "Devices"
            }
        }

        StackLayout {
            width: parent.width
            height: parent.height - 60
            currentIndex: tabBar.currentIndex

            Item {
                Label {
                    anchors.centerIn: parent
                    text: "File browser"
                    color: theme.textColorDim
                }
            }

            Item {
                Label {
                    anchors.centerIn: parent
                    text: "Plugin browser"
                    color: theme.textColorDim
                }
            }

            Item {
                Label {
                    anchors.centerIn: parent
                    text: "MIDI devices"
                    color: theme.textColorDim
                }
            }
        }
    }
}
