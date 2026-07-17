// SPDX-License-Identifier: GPL-3.0-or-later

#include "NetworkManagerAdapter.hpp"

#include <NetworkManagerQt/ActiveConnection>
#include <NetworkManagerQt/Connection>
#include <NetworkManagerQt/ConnectionSettings>
#include <NetworkManagerQt/Manager>
#include <NetworkManagerQt/Settings>
#include <QDBusError>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <algorithm>

namespace {

VpnState vpnState(NetworkManager::ActiveConnection::State state) {
    switch (state) {
    case NetworkManager::ActiveConnection::Activating:
        return VpnState::Connecting;
    case NetworkManager::ActiveConnection::Activated:
        return VpnState::Connected;
    case NetworkManager::ActiveConnection::Deactivating:
        return VpnState::Disconnecting;
    case NetworkManager::ActiveConnection::Deactivated:
        return VpnState::Disconnected;
    case NetworkManager::ActiveConnection::Unknown:
        return VpnState::Error;
    }

    return VpnState::Error;
}

bool isInUse(const NetworkManager::ActiveConnection::Ptr& connection) {
    return connection->state() != NetworkManager::ActiveConnection::Deactivated;
}

QString asynchronousError(const QDBusError& error, bool connecting) {
    const QString action = connecting ? QStringLiteral("connect to") : QStringLiteral("disconnect");
    if (error.type() == QDBusError::AccessDenied ||
        error.name() == QStringLiteral("org.freedesktop.NetworkManager.PermissionDenied")) {
        return QStringLiteral("NetworkManager denied permission to %1 the selected WireGuard "
                              "profile.")
            .arg(action);
    }
    if (error.type() == QDBusError::ServiceUnknown || error.type() == QDBusError::NoReply ||
        error.type() == QDBusError::Disconnected) {
        return QStringLiteral("NetworkManager became unavailable while trying to %1 the selected "
                              "WireGuard profile.")
            .arg(action);
    }

    return QStringLiteral("NetworkManager could not %1 the selected WireGuard profile.")
        .arg(action);
}

} // namespace

NetworkManagerAdapter::NetworkManagerAdapter(QObject* parent)
    : IVpnBackend(parent), m_servicePresent(!NetworkManager::version().isEmpty()),
      m_available(m_servicePresent) {
    auto* const notifier = NetworkManager::notifier();
    connect(notifier, &NetworkManager::Notifier::serviceDisappeared, this, [this]() {
        m_servicePresent = false;
        refreshProfiles();
    });
    connect(notifier, &NetworkManager::Notifier::serviceAppeared, this, [this]() {
        m_servicePresent = true;
        refreshProfiles();
    });

    m_refreshTimer.setInterval(1000);
    m_refreshTimer.setTimerType(Qt::CoarseTimer);
    connect(&m_refreshTimer, &QTimer::timeout, this, &NetworkManagerAdapter::refreshProfiles);
    m_refreshTimer.start();
}

bool NetworkManagerAdapter::isAvailable() const {
    return m_available;
}

void NetworkManagerAdapter::refreshProfiles() {
    const bool available = m_servicePresent && !NetworkManager::version().isEmpty();
    if (available != m_available) {
        m_available = available;
        Q_EMIT availabilityChanged(m_available);
    }

    if (!m_available) {
        m_pendingStates.clear();
        m_pendingOperationIds.clear();
        publishProfiles({});
        return;
    }

    QHash<QString, VpnState> activeStates;
    const auto activeConnections = NetworkManager::activeConnections();
    for (const auto& activeConnection : activeConnections) {
        if (!activeConnection || !activeConnection->isValid() ||
            activeConnection->type() != NetworkManager::ConnectionSettings::WireGuard ||
            activeConnection->uuid().isEmpty()) {
            continue;
        }

        activeStates.insert(activeConnection->uuid(), vpnState(activeConnection->state()));
    }

    VpnProfiles profiles;
    const auto connections = NetworkManager::listConnections();
    profiles.reserve(connections.size());
    for (const auto& connection : connections) {
        if (!connection || !connection->isValid()) {
            continue;
        }

        const auto settings = connection->settings();
        if (!settings ||
            settings->connectionType() != NetworkManager::ConnectionSettings::WireGuard ||
            connection->uuid().isEmpty() || connection->name().isEmpty()) {
            continue;
        }

        const QString uuid = connection->uuid();
        const VpnState state =
            m_pendingStates.value(uuid, activeStates.value(uuid, VpnState::Disconnected));
        profiles.append(VpnProfile{connection->name(), uuid, state});
    }

    std::sort(profiles.begin(), profiles.end(),
              [](const VpnProfile& left, const VpnProfile& right) {
                  const int nameComparison = QString::localeAwareCompare(left.name, right.name);
                  return nameComparison == 0 ? left.uuid < right.uuid : nameComparison < 0;
              });

    publishProfiles(profiles);
}

void NetworkManagerAdapter::connectProfile(const QString& uuid) {
    if (!m_available) {
        Q_EMIT errorOccurred(QStringLiteral("NetworkManager is unavailable."));
        return;
    }
    if (uuid.isEmpty()) {
        Q_EMIT errorOccurred(QStringLiteral("No WireGuard profile is selected."));
        return;
    }
    if (m_pendingStates.contains(uuid)) {
        Q_EMIT errorOccurred(
            QStringLiteral("An operation for this profile is already in progress."));
        return;
    }

    NetworkManager::Connection::Ptr selectedConnection;
    const auto connections = NetworkManager::listConnections();
    for (const auto& connection : connections) {
        if (connection && connection->isValid() && connection->uuid() == uuid) {
            selectedConnection = connection;
            break;
        }
    }

    if (!selectedConnection) {
        Q_EMIT errorOccurred(
            QStringLiteral("The selected NetworkManager profile no longer exists."));
        return;
    }

    const auto selectedSettings = selectedConnection->settings();
    if (!selectedSettings ||
        selectedSettings->connectionType() != NetworkManager::ConnectionSettings::WireGuard) {
        Q_EMIT errorOccurred(QStringLiteral("The selected profile is not a WireGuard profile."));
        return;
    }

    const auto activeConnections = NetworkManager::activeConnections();
    for (const auto& activeConnection : activeConnections) {
        if (!activeConnection || !activeConnection->isValid() ||
            activeConnection->type() != NetworkManager::ConnectionSettings::WireGuard ||
            !isInUse(activeConnection)) {
            continue;
        }
        if (activeConnection->uuid() == uuid) {
            return;
        }

        Q_EMIT errorOccurred(QStringLiteral(
            "Disconnect the active WireGuard profile before connecting another profile."));
        return;
    }

    if (!m_pendingStates.isEmpty()) {
        Q_EMIT errorOccurred(QStringLiteral("Wait for the current WireGuard operation to finish "
                                            "before connecting another profile."));
        return;
    }

    const quint64 operationId = ++m_nextOperationId;
    m_pendingStates.insert(uuid, VpnState::Connecting);
    m_pendingOperationIds.insert(uuid, operationId);
    refreshProfiles();
    if (!m_available || !m_pendingOperationIds.contains(uuid) ||
        m_pendingOperationIds.value(uuid) != operationId) {
        return;
    }

    const auto reply = NetworkManager::activateConnection(selectedConnection->path(),
                                                          QStringLiteral("/"), QStringLiteral("/"));
    auto* watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, uuid, operationId](QDBusPendingCallWatcher* completedCall) {
                if (!m_pendingOperationIds.contains(uuid) ||
                    m_pendingOperationIds.value(uuid) != operationId) {
                    completedCall->deleteLater();
                    return;
                }

                const QDBusPendingReply<QDBusObjectPath> result = *completedCall;
                m_pendingStates.remove(uuid);
                m_pendingOperationIds.remove(uuid);
                completedCall->deleteLater();
                if (result.isError()) {
                    Q_EMIT errorOccurred(asynchronousError(result.error(), true));
                }
                refreshProfiles();
            });
}

void NetworkManagerAdapter::disconnectProfile(const QString& uuid) {
    if (!m_available) {
        Q_EMIT errorOccurred(QStringLiteral("NetworkManager is unavailable."));
        return;
    }
    if (uuid.isEmpty()) {
        Q_EMIT errorOccurred(QStringLiteral("No WireGuard profile is selected."));
        return;
    }
    if (m_pendingStates.contains(uuid)) {
        Q_EMIT errorOccurred(
            QStringLiteral("An operation for this profile is already in progress."));
        return;
    }

    NetworkManager::ActiveConnection::Ptr selectedConnection;
    const auto activeConnections = NetworkManager::activeConnections();
    for (const auto& activeConnection : activeConnections) {
        if (activeConnection && activeConnection->isValid() &&
            activeConnection->type() == NetworkManager::ConnectionSettings::WireGuard &&
            activeConnection->uuid() == uuid && isInUse(activeConnection)) {
            selectedConnection = activeConnection;
            break;
        }
    }

    if (!selectedConnection) {
        Q_EMIT errorOccurred(
            QStringLiteral("The selected WireGuard profile is not currently active."));
        return;
    }

    const quint64 operationId = ++m_nextOperationId;
    m_pendingStates.insert(uuid, VpnState::Disconnecting);
    m_pendingOperationIds.insert(uuid, operationId);
    refreshProfiles();
    if (!m_available || !m_pendingOperationIds.contains(uuid) ||
        m_pendingOperationIds.value(uuid) != operationId) {
        return;
    }

    const auto reply = NetworkManager::deactivateConnection(selectedConnection->path());
    auto* watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, uuid, operationId](QDBusPendingCallWatcher* completedCall) {
                if (!m_pendingOperationIds.contains(uuid) ||
                    m_pendingOperationIds.value(uuid) != operationId) {
                    completedCall->deleteLater();
                    return;
                }

                const QDBusPendingReply<> result = *completedCall;
                m_pendingStates.remove(uuid);
                m_pendingOperationIds.remove(uuid);
                completedCall->deleteLater();
                if (result.isError()) {
                    Q_EMIT errorOccurred(asynchronousError(result.error(), false));
                }
                refreshProfiles();
            });
}

void NetworkManagerAdapter::publishProfiles(const VpnProfiles& profiles) {
    if (m_lastProfiles == profiles) {
        return;
    }

    m_lastProfiles = profiles;
    Q_EMIT profilesChanged(m_lastProfiles);
}
