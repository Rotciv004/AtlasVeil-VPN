// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/IVpnBackend.hpp"

#include <QHash>
#include <QString>
#include <QTimer>
#include <QtGlobal>

class NetworkManagerAdapter final : public IVpnBackend {
    Q_OBJECT

  public:
    explicit NetworkManagerAdapter(QObject* parent = nullptr);

    [[nodiscard]] bool isAvailable() const override;

  public Q_SLOTS:
    void refreshProfiles() override;
    void connectProfile(const QString& uuid) override;
    void disconnectProfile(const QString& uuid) override;

  private:
    void publishProfiles(const VpnProfiles& profiles);

    bool m_servicePresent;
    bool m_available;
    VpnProfiles m_lastProfiles;
    QHash<QString, VpnState> m_pendingStates;
    quint64 m_nextOperationId{0};
    QHash<QString, quint64> m_pendingOperationIds;
    QTimer m_refreshTimer;
};
