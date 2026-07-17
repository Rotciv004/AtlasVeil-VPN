// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/VpnController.hpp"
#include "network/NetworkManagerAdapter.hpp"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <cstdlib>

int main(int argc, char* argv[]) {
    QGuiApplication application(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("AtlasVeil"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QCoreApplication::setOrganizationName(QStringLiteral("AtlasVeil"));
    QGuiApplication::setDesktopFileName(QStringLiteral("io.github.rotciv004.atlasveil"));

    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }

    NetworkManagerAdapter backend;
    VpnController controller(backend);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("vpnController"), &controller);
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &application,
        []() { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);

    controller.refresh();
    engine.loadFromModule(QStringLiteral("AtlasVeil"), QStringLiteral("Main"));

    return application.exec();
}
