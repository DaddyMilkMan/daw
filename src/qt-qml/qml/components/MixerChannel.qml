/**
 * Mixer Channel - Individual channel strip in mixer
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../styles"

Rectangle {
    id: mixerChannel

    property var trackData: null

    color: theme.surfaceColor
    border.color: theme.borderColor
    border.width: 1
    radius: theme.radiusSmall

    AppTheme { id: theme }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        Label {
            text: trackData ? trackData.name : "Channel"
            font.pixelSize: 12
            font.bold: true
            color: theme.textColor
            Layout.alignment: Qt.AlignHCenter
        }

        // Volume fader
        Slider {
            id: volumeSlider
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignHCenter
            orientation: Qt.Vertical
            from: 0.0
            to: 1.0
            value: trackData ? trackData.volume : 0.75

            onValueChanged: {
                if (trackData && pressed) {
                    audioEngine.setTrackVolume(trackData.id, value)
                }
            }
        }

        // Pan knob
        Dial {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 50
            Layout.preferredHeight: 50
            from: -1.0
            to: 1.0
            value: trackData ? trackData.pan : 0.0

            onValueChanged: {
                if (trackData && pressed) {
                    audioEngine.setTrackPan(trackData.id, value)
                }
            }
        }

        Label {
            text: "Pan: " + (trackData ? trackData.pan.toFixed(2) : "0.00")
            font.pixelSize: 10
            color: theme.textColorDim
            Layout.alignment: Qt.AlignHCenter
        }
    }
}
