/**
 * Mixer Panel - Track mixer with faders and controls
 */

import QtQuick
import QtQuick.Controls
import "../styles"

Rectangle {
    id: mixerPanel
    color: theme.panelColor

    AppTheme { id: theme }

    Column {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Label {
            text: "MIXER"
            font.pixelSize: 16
            font.bold: true
            color: theme.textColor
        }

        ListView {
            width: parent.width
            height: parent.height - 40
            model: audioEngine.tracks
            spacing: 10
            clip: true

            delegate: MixerChannel {
                width: ListView.view.width
                height: 200
                trackData: modelData
            }

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }
        }
    }
}
