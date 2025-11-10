/**
 * App Theme - Color scheme and styling constants
 */

import QtQuick

QtObject {
    // Background colors
    readonly property color backgroundColor: "#1a1a1a"
    readonly property color panelColor: "#2a2a2a"
    readonly property color surfaceColor: "#333333"
    readonly property color surfaceColorLight: "#3a3a3a"

    // Text colors
    readonly property color textColor: "#ffffff"
    readonly property color textColorDim: "#999999"
    readonly property color textColorDisabled: "#666666"

    // Accent colors
    readonly property color primaryColor: "#4CAF50"      // Green
    readonly property color primaryColorDark: "#388E3C"
    readonly property color secondaryColor: "#00BCD4"    // Cyan
    readonly property color accentColor: "#FFC107"       // Amber

    // Status colors
    readonly property color successColor: "#4CAF50"
    readonly property color warningColor: "#FFC107"
    readonly property color errorColor: "#f44336"
    readonly property color infoColor: "#2196F3"

    // UI element colors
    readonly property color borderColor: "#444444"
    readonly property color dividerColor: "#333333"
    readonly property color hoverColor: "#3a3a3a"
    readonly property color activeColor: "#4a4a4a"
    readonly property color selectedColor: "#1976D2"

    // Track colors (for track coloring)
    readonly property var trackColors: [
        "#f44336", "#E91E63", "#9C27B0", "#673AB7",
        "#3F51B5", "#2196F3", "#03A9F4", "#00BCD4",
        "#009688", "#4CAF50", "#8BC34A", "#CDDC39",
        "#FFEB3B", "#FFC107", "#FF9800", "#FF5722"
    ]

    // Fonts
    readonly property int fontSizeSmall: 10
    readonly property int fontSizeNormal: 12
    readonly property int fontSizeLarge: 14
    readonly property int fontSizeTitle: 18
    readonly property int fontSizeHeader: 24

    // Spacing
    readonly property int spacingSmall: 4
    readonly property int spacingNormal: 8
    readonly property int spacingLarge: 16
    readonly property int spacingXLarge: 24

    // Radius
    readonly property int radiusSmall: 4
    readonly property int radiusNormal: 6
    readonly property int radiusLarge: 8

    // Animation durations
    readonly property int animationFast: 100
    readonly property int animationNormal: 200
    readonly property int animationSlow: 300
}
