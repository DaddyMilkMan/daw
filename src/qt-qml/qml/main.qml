/**
 * Zenith DAW - Main Application Window
 *
 * This is the main QML entry point for the DAW UI.
 * It defines the overall application layout with:
 * - Top bar (transport controls, tempo, time signature)
 * - Left sidebar (browser, devices)
 * - Center panel (tracks, arrangement view)
 * - Right panel (mixer, effects)
 * - Bottom bar (timeline, waveform)
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

import "components"
import "styles"

ApplicationWindow {
    id: mainWindow

    width: 1920
    height: 1080
    visible: true
    title: "Zenith DAW - Professional Digital Audio Workstation"

    // Dark theme colors
    color: "#1a1a1a"

    // App theme singleton
    AppTheme {
        id: theme
    }

    // Keyboard shortcuts
    Shortcut {
        sequence: "Space"
        onActivated: {
            if (audioEngine.isPlaying) {
                audioEngine.stop()
            } else {
                audioEngine.play()
            }
        }
    }

    Shortcut {
        sequence: "Ctrl+N"
        onActivated: audioEngine.newProject()
    }

    Shortcut {
        sequence: "Ctrl+S"
        onActivated: {
            // TODO: Show save dialog
            console.log("Save project")
        }
    }

    Shortcut {
        sequence: "Ctrl+Z"
        onActivated: audioEngine.undo()
    }

    Shortcut {
        sequence: "Ctrl+Y"
        onActivated: audioEngine.redo()
    }

    // Main layout
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Top bar with transport controls
        TopBar {
            id: topBar
            Layout.fillWidth: true
            Layout.preferredHeight: 70
            Layout.minimumHeight: 70
        }

        // Main content area
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Left sidebar - Browser and devices
            SidebarPanel {
                id: sidebar
                Layout.preferredWidth: 280
                Layout.minimumWidth: 200
                Layout.maximumWidth: 400
                Layout.fillHeight: true
            }

            // Resizer for sidebar
            Rectangle {
                Layout.preferredWidth: 2
                Layout.fillHeight: true
                color: theme.borderColor

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.SizeHorCursor
                    drag.target: parent
                    drag.axis: Drag.XAxis
                }
            }

            // Center panel - Tracks and arrangement
            TrackView {
                id: trackView
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            // Resizer for mixer
            Rectangle {
                Layout.preferredWidth: 2
                Layout.fillHeight: true
                color: theme.borderColor

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.SizeHorCursor
                }
            }

            // Right panel - Mixer
            MixerPanel {
                id: mixerPanel
                Layout.preferredWidth: 400
                Layout.minimumWidth: 300
                Layout.maximumWidth: 600
                Layout.fillHeight: true
            }
        }

        // Bottom separator
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 2
            color: theme.borderColor
        }

        // Bottom bar - Timeline and waveform
        TransportBar {
            id: transportBar
            Layout.fillWidth: true
            Layout.preferredHeight: 150
            Layout.minimumHeight: 100
        }
    }

    // Status bar at the very bottom
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 24
        color: theme.backgroundColor

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            spacing: 15

            Label {
                text: audioEngine.isRecording ? "● REC" : (audioEngine.isPlaying ? "▶ PLAY" : "■ STOP")
                color: audioEngine.isRecording ? "#f44336" : (audioEngine.isPlaying ? "#4CAF50" : "#999999")
                font.pixelSize: 11
                font.bold: true
            }

            Rectangle {
                width: 1
                height: 16
                color: theme.borderColor
            }

            Label {
                text: "Bar: " + Math.floor(audioEngine.currentBar + 1)
                color: theme.textColor
                font.pixelSize: 11
            }

            Rectangle {
                width: 1
                height: 16
                color: theme.borderColor
            }

            Label {
                text: audioEngine.tempo.toFixed(2) + " BPM"
                color: theme.textColor
                font.pixelSize: 11
            }

            Rectangle {
                width: 1
                height: 16
                color: theme.borderColor
            }

            Label {
                text: audioEngine.numerator + "/" + audioEngine.denominator
                color: theme.textColor
                font.pixelSize: 11
            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                text: "CPU: 12%"
                color: theme.textColor
                font.pixelSize: 11
            }

            Rectangle {
                width: 1
                height: 16
                color: theme.borderColor
            }

            Label {
                text: "Latency: 5.3ms"
                color: theme.textColor
                font.pixelSize: 11
            }
        }
    }

    // Welcome dialog (shown on first start)
    Loader {
        id: welcomeDialogLoader
        anchors.centerIn: parent
        active: false
        sourceComponent: Rectangle {
            width: 600
            height: 400
            color: theme.panelColor
            radius: 8
            border.color: theme.borderColor
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 40
                spacing: 20

                Label {
                    text: "Welcome to Zenith DAW"
                    font.pixelSize: 28
                    font.bold: true
                    color: theme.textColor
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    text: "Professional Digital Audio Workstation"
                    font.pixelSize: 14
                    color: theme.textColorDim
                    Layout.alignment: Qt.AlignHCenter
                }

                Item {
                    Layout.fillHeight: true
                }

                Button {
                    text: "Create New Project"
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 200
                    Layout.preferredHeight: 50

                    onClicked: {
                        audioEngine.newProject()
                        welcomeDialogLoader.active = false
                    }
                }

                Button {
                    text: "Open Existing Project"
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 200
                    Layout.preferredHeight: 50

                    onClicked: {
                        // TODO: Show file open dialog
                        welcomeDialogLoader.active = false
                    }
                }

                Item {
                    Layout.fillHeight: true
                }
            }
        }
    }

    // Notification system
    Column {
        id: notificationArea
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 20
        spacing: 10
        z: 1000

        // Notifications will be dynamically added here
    }

    // Connect to audio engine signals
    Connections {
        target: audioEngine

        function onErrorOccurred(message) {
            console.error("Audio Engine Error:", message)
            showNotification(message, "error")
        }

        function onWarningOccurred(message) {
            console.warn("Audio Engine Warning:", message)
            showNotification(message, "warning")
        }
    }

    // Helper function to show notifications
    function showNotification(message, type) {
        var component = Qt.createComponent("components/Notification.qml")
        if (component.status === Component.Ready) {
            var notification = component.createObject(notificationArea, {
                "message": message,
                "notificationType": type
            })
        }
    }

    // Component initialization
    Component.onCompleted: {
        console.log("Zenith DAW main window loaded")
        console.log("Qt version:", Qt.application.version)
        console.log("Audio engine initialized:", audioEngine !== null)
    }
}
