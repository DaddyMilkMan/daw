/**
 * Plugin Rack - VST/AU plugin list for a track
 */

import QtQuick
import QtQuick.Controls
import "../styles"

Rectangle {
    id: pluginRack
    color: theme.panelColor

    AppTheme { id: theme }

    Label {
        anchors.centerIn: parent
        text: "Plugin Rack (Coming Soon)"
        color: theme.textColorDim
    }
}
