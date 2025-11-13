/**
 * Timeline - Arrangement timeline with bars and beats
 */

import QtQuick
import QtQuick.Controls
import "../styles"

Rectangle {
    id: timeline
    color: theme.surfaceColor

    AppTheme { id: theme }

    Label {
        anchors.centerIn: parent
        text: "Timeline (Coming Soon)"
        color: theme.textColorDim
    }
}
