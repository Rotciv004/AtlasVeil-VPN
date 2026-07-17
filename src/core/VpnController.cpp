// SPDX-License-Identifier: GPL-3.0-or-later

#include "VpnController.hpp"

#include <QVariantMap>
#include <algorithm>

VpnController::VpnController(IVpnBackend& backend, QObject* parent)
    : QObject(parent), m_backend(backend), m_backendAvailable(backend.isAvailable()) {
    connect(&m_backend, &IVpnBackend::availabilityChanged, this,
            &VpnController::handleAvailabilityChanged);
    connect(&m_backend, &IVpnBackend::profilesChanged, this, &VpnController::handleProfilesChanged);
    connect(&m_backend, &IVpnBackend::errorOccurred, this, &VpnController::handleErrorOccurred);
}

bool VpnController::backendAvailable() const {
    return m_backendAvailable;
}

QVariantList VpnController::profiles() const {
    QVariantList result;
    result.reserve(m_profiles.size());

    for (const VpnProfile& profile : m_profiles) {
        result.append(QVariantMap{{QStringLiteral("name"), profile.name},
                                  {QStringLiteral("uuid"), profile.uuid},
                                  {QStringLiteral("state"), static_cast<int>(profile.state)},
                                  {QStringLiteral("stateText"), stateToText(profile.state)}});
    }

    return result;
}

QString VpnController::selectedProfileUuid() const {
    return m_selectedProfileUuid;
}

void VpnController::setSelectedProfileUuid(const QString& uuid) {
    if (uuid == m_selectedProfileUuid) {
        return;
    }

    const auto profile = std::find_if(m_profiles.cbegin(), m_profiles.cend(),
                                      [&uuid](const auto& item) { return item.uuid == uuid; });
    if (profile == m_profiles.cend()) {
        return;
    }

    const DerivedState previous = derivedState();
    m_selectedProfileUuid = uuid;
    Q_EMIT selectedProfileUuidChanged();
    emitDerivedChanges(previous);
}

QString VpnController::stateText() const {
    return derivedState().stateText;
}

QString VpnController::lastError() const {
    return m_lastError;
}

bool VpnController::busy() const {
    return derivedState().busy;
}

bool VpnController::canConnect() const {
    return derivedState().canConnect;
}

bool VpnController::canDisconnect() const {
    return derivedState().canDisconnect;
}

void VpnController::refresh() {
    m_backend.refreshProfiles();
}

void VpnController::connectSelected() {
    if (!canConnect()) {
        return;
    }

    clearError();
    m_backend.connectProfile(m_selectedProfileUuid);
}

void VpnController::disconnectSelected() {
    if (!canDisconnect()) {
        return;
    }

    clearError();
    m_backend.disconnectProfile(m_selectedProfileUuid);
}

void VpnController::clearError() {
    if (m_lastError.isEmpty()) {
        return;
    }

    m_lastError.clear();
    Q_EMIT lastErrorChanged();
}

VpnController::DerivedState VpnController::derivedState() const {
    const VpnProfile* selected = selectedProfile();
    const bool operationInProgress =
        std::any_of(m_profiles.cbegin(), m_profiles.cend(), [](const VpnProfile& profile) {
            return profile.state == VpnState::Connecting ||
                   profile.state == VpnState::Disconnecting;
        });
    const bool allDisconnected =
        std::all_of(m_profiles.cbegin(), m_profiles.cend(), [](const VpnProfile& profile) {
            return profile.state == VpnState::Disconnected;
        });

    DerivedState result;
    result.stateText =
        selected != nullptr ? stateToText(selected->state) : QStringLiteral("No profile selected");
    result.busy = operationInProgress;
    result.canConnect = m_backendAvailable && selected != nullptr &&
                        selected->state == VpnState::Disconnected && allDisconnected;
    result.canDisconnect =
        m_backendAvailable && selected != nullptr &&
        (selected->state == VpnState::Connected || selected->state == VpnState::Error);
    return result;
}

void VpnController::emitDerivedChanges(const DerivedState& previous) {
    const DerivedState current = derivedState();
    if (current.stateText != previous.stateText) {
        Q_EMIT stateTextChanged();
    }
    if (current.busy != previous.busy) {
        Q_EMIT busyChanged();
    }
    if (current.canConnect != previous.canConnect) {
        Q_EMIT canConnectChanged();
    }
    if (current.canDisconnect != previous.canDisconnect) {
        Q_EMIT canDisconnectChanged();
    }
}

void VpnController::handleAvailabilityChanged(bool available) {
    if (m_backendAvailable == available) {
        return;
    }

    const DerivedState previous = derivedState();
    m_backendAvailable = available;
    Q_EMIT backendAvailableChanged();
    emitDerivedChanges(previous);
}

void VpnController::handleProfilesChanged(const VpnProfiles& profiles) {
    if (m_profiles == profiles) {
        return;
    }

    const DerivedState previous = derivedState();
    const QString previousSelection = m_selectedProfileUuid;
    m_profiles = profiles;

    const auto selected =
        std::find_if(m_profiles.cbegin(), m_profiles.cend(), [this](const VpnProfile& profile) {
            return profile.uuid == m_selectedProfileUuid;
        });
    if (selected == m_profiles.cend()) {
        m_selectedProfileUuid = m_profiles.isEmpty() ? QString{} : m_profiles.constFirst().uuid;
    }

    Q_EMIT profilesChanged();
    if (m_selectedProfileUuid != previousSelection) {
        Q_EMIT selectedProfileUuidChanged();
    }
    emitDerivedChanges(previous);
}

void VpnController::handleErrorOccurred(const QString& message) {
    if (m_lastError == message) {
        return;
    }

    m_lastError = message;
    Q_EMIT lastErrorChanged();
}

const VpnProfile* VpnController::selectedProfile() const {
    const auto selected =
        std::find_if(m_profiles.cbegin(), m_profiles.cend(), [this](const VpnProfile& profile) {
            return profile.uuid == m_selectedProfileUuid;
        });
    return selected != m_profiles.cend() ? &*selected : nullptr;
}

QString VpnController::stateToText(VpnState state) {
    switch (state) {
    case VpnState::Disconnected:
        return QStringLiteral("Disconnected");
    case VpnState::Connecting:
        return QStringLiteral("Connecting");
    case VpnState::Connected:
        return QStringLiteral("Connected");
    case VpnState::Disconnecting:
        return QStringLiteral("Disconnecting");
    case VpnState::Error:
        return QStringLiteral("Error");
    }

    return QStringLiteral("Error");
}
