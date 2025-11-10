/**
 * Top Bar - Transport controls, tempo, time signature
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../styles"

Rectangle {
    id: topBar
    color: theme.panelColor

    AppTheme { id: theme }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 20

        // Logo/Title
        Label {
            text: "VEXEL"
            font.pixelSize: 20
            font.bold: true
            color: theme.primaryColor
        }

        Rectangle {
            width: 1
            height: 40
            color: theme.borderColor
        }

        // Transport controls
        Row {
            spacing: 5

            Button {
                text: "⏮"
                width: 40
                height: 40
                onClicked: audioEngine.rewind()
            }

            Button {
                text: audioEngine.isPlaying ? "⏸" : "▶"
                width: 50
                height: 40
                highlighted: audioEngine.isPlaying
                onClicked: {
                    if (audioEngine.isPlaying) audioEngine.pause()
                    else audioEngine.play()
                }
            }

            Button {
                text: "⏹"
                width: 40
                height: 40
                onClicked: audioEngine.stop()
            }

            Button {
                text: "●"
                width: 40
                height: 40
                highlighted: audioEngine.isRecording
                onClicked: audioEngine.record()
            }

            Button {
                text: "⏭"
                width: 40
                height: 40
                onClicked: audioEngine.fastForward()
            }
        }

        Rectangle {
            width: 1
            height: 40
            color: theme.borderColor
        }

        // Tempo control
        Column {
            spacing: 2

            Label {
                text: "TEMPO"
                font.pixelSize: 10
                color: theme.textColorDim
            }

            SpinBox {
                from: 20
                to: 999
                value: audioEngine.tempo
                editable: true

                onValueChanged: {
                    if (value !== audioEngine.tempo) {
                        audioEngine.setTempo(value)
                    }
                }
            }
        }

        // Time signature
        Column {
            spacing: 2

            Label {
                text: "TIME SIG"
                font.pixelSize: 10
                color: theme.textColorDim
            }

            Row {
                spacing: 5

                SpinBox {
                    width: 60
                    from: 1
                    to: 16
                    value: audioEngine.numerator
                    onValueModified: audioEngine.setNumerator(value)
                }

                Label {
                    text: "/"
                    anchors.verticalCenter: parent.verticalCenter
                }

                SpinBox {
                    width: 60
                    from: 1
                    to: 16
                    value: audioEngine.denominator
                    onValueModified: audioEngine.setDenominator(value)
                }
            }
        }

        Rectangle {
            width: 1
            height: 40
            color: theme.borderColor
        }

        // Loop toggle
        Button {
            text: "LOOP"
            checkable: true
            checked: audioEngine.loopEnabled
            onClicked: audioEngine.setLoopEnabled(checked)
        }

        // Metronome toggle
        Button {
            text: "METRO"
            checkable: true
            checked: audioEngine.metronomeEnabled
            onClicked: audioEngine.setMetronomeEnabled(checked)
        }

        Item {
            Layout.fillWidth: true
        }

        // Master volume
        Column {
            spacing: 2

            Label {
                text: "MASTER"
                font.pixelSize: 10
                color: theme.textColorDim
            }

            Slider {
                width: 120
                from: 0.0
                to: 1.0
                value: audioEngine.masterVolume
                onValueChanged: audioEngine.setMasterVolume(value)
            }
        }
    }
}
