/**
 * Vexel DAW - Qt/QML Main Application Entry Point
 *
 * This is the main entry point for the Qt/QML-based DAW application.
 * It initializes the Qt application, creates the QML engine, and sets up
 * the bridge to the JUCE audio engine.
 */

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QQuickStyle>
#include <QFont>

#include "bridge/AudioEngineInterface.h"

int main(int argc, char *argv[])
{
    // Enable high DPI scaling
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    // Create Qt application
    QGuiApplication app(argc, argv);

    // Application metadata
    app.setOrganizationName("Vexel");
    app.setOrganizationDomain("vexel.com");
    app.setApplicationName("Vexel DAW");
    app.setApplicationVersion("1.0.0");

    // Set application icon
    // app.setWindowIcon(QIcon(":/icons/app-icon.png"));

    // Set Qt Quick style (Material, Fusion, etc.)
    QQuickStyle::setStyle("Fusion");

    // Set default font
    QFont appFont("Inter", 10);
    app.setFont(appFont);

    // Create audio engine interface
    // This bridges between QML UI and JUCE audio engine
    AudioEngineInterface audioEngine;

    // Initialize audio engine
    if (!audioEngine.initialize()) {
        qCritical() << "Failed to initialize audio engine";
        return 1;
    }

    // Create QML engine
    QQmlApplicationEngine engine;

    // Expose audio engine to QML context
    // Now QML can access it via "audioEngine" global property
    engine.rootContext()->setContextProperty("audioEngine", &audioEngine);

    // Load main QML file
    const QUrl url(QStringLiteral("qrc:/qt/qml/VexelDAW/qml/main.qml"));

    // Connect to objectCreated signal for error handling
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                qCritical() << "Failed to load QML file:" << url;
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection
    );

    // Load QML
    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "No root objects found in QML";
        return 1;
    }

    qInfo() << "Vexel DAW started successfully";
    qInfo() << "Qt version:" << QT_VERSION_STR;
    qInfo() << "Audio engine initialized";

    // Run application event loop
    return app.exec();
}
