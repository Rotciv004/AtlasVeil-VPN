// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/IVpnBackend.hpp"

class FakeVpnBackend final : public IVpnBackend {
  public:
    explicit FakeVpnBackend(QObject* parent = nullptr) : IVpnBackend(parent) {}

    [[nodiscard]] bool isAvailable() const override {
        return available;
    }

    void refreshProfiles() override {
        ++refreshCallCount;
    }

    void connectProfile(const QString& uuid) override {
        ++connectCallCount;
        lastConnectedUuid = uuid;
    }

    void disconnectProfile(const QString& uuid) override {
        ++disconnectCallCount;
        lastDisconnectedUuid = uuid;
    }

    void publishProfiles(const VpnProfiles& newProfiles) {
        currentProfiles = newProfiles;
        emit profilesChanged(currentProfiles);
    }

    void publishAvailability(bool newAvailability) {
        available = newAvailability;
        emit availabilityChanged(available);
    }

    void publishError(const QString& message) {
        emit errorOccurred(message);
    }

    bool available{true};
    VpnProfiles currentProfiles;
    int refreshCallCount{0};
    int connectCallCount{0};
    int disconnectCallCount{0};
    QString lastConnectedUuid;
    QString lastDisconnectedUuid;
};
