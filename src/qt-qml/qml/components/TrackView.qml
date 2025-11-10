/**
 * Track View - Main arrangement view with tracks
 */

import QtQuick
import QtQuick.Controls
import "../styles"

Rectangle {
    id: trackView
    color: theme.backgroundColor

    AppTheme { id: theme }

    ListView {
        id: trackList
        anchors.fill: parent
        model: audioEngine.tracks
        spacing: 2
        clip: true

        delegate: TrackItem {
            width: trackList.width
            height: 100
            trackData: modelData
        }

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        // Empty state
        Label {
            anchors.centerIn: parent
            visible: trackList.count === 0
            text: "No tracks\nClick '+' to add a track"
            color: theme.textColorDim
            font.pixelSize: 16
            horizontalAlignment: Text.AlignHCenter
        }
    }

    // Add track button (floating)
    RoundButton {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 20
        text: "+"
        width: 56
        height: 56
        font.pixelSize: 24

        onClicked: {
            audioEngine.addTrack("New Track", true)
        }
    }
}
