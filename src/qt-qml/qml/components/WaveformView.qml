/**
 * Waveform View - GPU-accelerated waveform visualization
 */

import QtQuick
import "../styles"

Canvas {
    id: waveformCanvas

    AppTheme { id: theme }

    property var waveformData: audioEngine.currentWaveform
    property color waveColor: theme.secondaryColor

    renderStrategy: Canvas.Threaded
    renderTarget: Canvas.FramebufferObject

    onWaveformDataChanged: requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()

        // Background
        ctx.fillStyle = theme.backgroundColor
        ctx.fillRect(0, 0, width, height)

        // Grid
        ctx.strokeStyle = theme.dividerColor
        ctx.lineWidth = 1
        for (var i = 0; i < 10; i++) {
            var y = (height / 10) * i
            ctx.beginPath()
            ctx.moveTo(0, y)
            ctx.lineTo(width, y)
            ctx.stroke()
        }

        // Waveform
        if (!waveformData || waveformData.length === 0) return

        var gradient = ctx.createLinearGradient(0, 0, 0, height)
        gradient.addColorStop(0, Qt.lighter(waveColor, 1.5))
        gradient.addColorStop(0.5, waveColor)
        gradient.addColorStop(1, Qt.darker(waveColor, 1.2))

        ctx.strokeStyle = gradient
        ctx.lineWidth = 2
        ctx.beginPath()

        for (var x = 0; x < width; x++) {
            var sampleIndex = Math.floor((x / width) * waveformData.length)
            if (sampleIndex >= waveformData.length) break

            var sample = waveformData[sampleIndex]
            var y = (1 - sample) * height / 2 + height / 2

            if (x === 0) ctx.moveTo(x, y)
            else ctx.lineTo(x, y)
        }

        ctx.stroke()
    }
}
