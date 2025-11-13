/**
 * Track Item - Individual track in the arrangement view
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../styles"

Rectangle {
    id: trackItem

    property var trackData: null

    color: trackData && trackData.solo ? Qt.darker(theme.surfaceColor, 0.9) : theme.surfaceColor
    border.color: theme.borderColor
    border.width: 1
    radius: theme.radiusSmall

    AppTheme { id: theme }

    // Smooth appear animation
    opacity: 0
    Component.onCompleted: {
        opacityAnimation.start()
    }

    NumberAnimation on opacity {
        id: opacityAnimation
        from: 0
        to: 1
        duration: theme.animationNormal
        easing.type: Easing.OutQuad
    }

    // Hover effect
    Behavior on color {
        ColorAnimation { duration: theme.animationFast }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onEntered: {
            trackItem.color = Qt.lighter(trackItem.color, 1.1)
        }

        onExited: {
            trackItem.color = trackData && trackData.solo ? Qt.darker(theme.surfaceColor, 0.9) : theme.surfaceColor
        }

        onClicked: (mouse) => {
            if (mouse.button === Qt.RightButton) {
                trackContextMenu.popup()
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        // Track color indicator
        Rectangle {
            width: 6
            Layout.fillHeight: true
            color: trackData ? trackData.color : theme.primaryColor
            radius: 3
        }

        // Track info
        Column {
            Layout.fillWidth: true
            spacing: 5

            Label {
                text: trackData ? trackData.name : "Track"
                font.pixelSize: 14
                font.bold: true
                color: theme.textColor
            }

            Label {
                text: trackData ? (trackData.isAudio ? "Audio Track" : "MIDI Track") : ""
                font.pixelSize: 10
                color: theme.textColorDim
            }

            // Level meter
            Rectangle {
                width: parent.width
                height: 20
                color: theme.backgroundColor
                radius: theme.radiusSmall

                Rectangle {
                    id: levelMeter
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: trackData ? parent.width * trackData.levelMeter : 0
                    color: {
                        if (!trackData) return theme.primaryColor
                        var level = trackData.levelMeter
                        if (level > 0.9) return theme.errorColor
                        if (level > 0.7) return theme.warningColor
                        return theme.successColor
                    }
                    radius: theme.radiusSmall

                    Behavior on width {
                        SmoothedAnimation { velocity: 500 }
                    }

                    Behavior on color {
                        ColorAnimation { duration: theme.animationFast }
                    }
                }
            }
        }

        // Track controls
        Row {
            spacing: 5

            Button {
                text: "M"
                width: 30
                height: 30
                checkable: true
                checked: trackData ? trackData.muted : false
                onClicked: audioEngine.setTrackMute(trackData.id, checked)

                background: Rectangle {
                    color: parent.checked ? theme.errorColor : theme.surfaceColorLight
                    radius: theme.radiusSmall

                    Behavior on color {
                        ColorAnimation { duration: theme.animationFast }
                    }
                }
            }

            Button {
                text: "S"
                width: 30
                height: 30
                checkable: true
                checked: trackData ? trackData.solo : false
                onClicked: audioEngine.setTrackSolo(trackData.id, checked)

                background: Rectangle {
                    color: parent.checked ? theme.warningColor : theme.surfaceColorLight
                    radius: theme.radiusSmall

                    Behavior on color {
                        ColorAnimation { duration: theme.animationFast }
                    }
                }
            }

            Button {
                text: "R"
                width: 30
                height: 30
                checkable: true
                checked: trackData ? trackData.armed : false
                onClicked: audioEngine.setTrackRecordArm(trackData.id, checked)

                background: Rectangle {
                    color: parent.checked ? theme.errorColor : theme.surfaceColorLight
                    radius: theme.radiusSmall

                    Behavior on color {
                        ColorAnimation { duration: theme.animationFast }
                    }
                }
            }
        }
    }

    // Context menu
    Menu {
        id: trackContextMenu

        MenuItem {
            text: "Rename Track"
            onTriggered: {
                // TODO: Show rename dialog
                console.log("Rename track")
            }
        }

        MenuItem {
            text: "Duplicate Track"
            onTriggered: {
                if (trackData) audioEngine.duplicateTrack(trackData.id)
            }
        }

        MenuItem {
            text: "Change Color"
            onTriggered: {
                // TODO: Show color picker
                console.log("Change color")
            }
        }

        MenuSeparator {}

        MenuItem {
            text: "Delete Track"
            onTriggered: {
                if (trackData) audioEngine.removeTrack(trackData.id)
            }
        }
    }
}
