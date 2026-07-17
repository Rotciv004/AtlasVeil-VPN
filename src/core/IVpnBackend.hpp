// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "VpnProfile.hpp"

#include <QObject>
#include <QString>

class IVpnBackend : public QObject {
    Q_OBJECT

  public:
    explicit IVpnBackend(QObject* parent = nullptr) : QObject(parent) {}

    ~IVpnBackend() override = default;

    [[nodiscard]] virtual bool isAvailable() const = 0;

  public Q_SLOTS:
    virtual void refreshProfiles() = 0;
    virtual void connectProfile(const QString& uuid) = 0;
    virtual void disconnectProfile(const QString& uuid) = 0;

  Q_SIGNALS:
    void availabilityChanged(bool available);
    void profilesChanged(const VpnProfiles& profiles);
    void errorOccurred(const QString& message);
};
