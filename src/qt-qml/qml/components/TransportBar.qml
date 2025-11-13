/**
 * Transport Bar - Timeline and waveform display
 */

import QtQuick
import QtQuick.Controls
import "../styles"

Rectangle {
    id: transportBar
    color: theme.panelColor

    AppTheme { id: theme }

    Column {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Label {
            text: "TIMELINE"
            font.pixelSize: 12
            font.bold: true
            color: theme.textColor
        }

        WaveformView {
            width: parent.width
            height: parent.height - 30
        }
    }
}
