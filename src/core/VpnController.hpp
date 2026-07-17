// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "IVpnBackend.hpp"
#include "VpnProfile.hpp"

#include <QObject>
#include <QString>
#include <QVariantList>

class VpnController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool backendAvailable READ backendAvailable NOTIFY backendAvailableChanged FINAL)
    Q_PROPERTY(QVariantList profiles READ profiles NOTIFY profilesChanged FINAL)
    Q_PROPERTY(QString selectedProfileUuid READ selectedProfileUuid WRITE setSelectedProfileUuid
                   NOTIFY selectedProfileUuidChanged FINAL)
    Q_PROPERTY(QString stateText READ stateText NOTIFY stateTextChanged FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged FINAL)
    Q_PROPERTY(bool canConnect READ canConnect NOTIFY canConnectChanged FINAL)
    Q_PROPERTY(bool canDisconnect READ canDisconnect NOTIFY canDisconnectChanged FINAL)

  public:
    explicit VpnController(IVpnBackend& backend, QObject* parent = nullptr);

    [[nodiscard]] bool backendAvailable() const;
    [[nodiscard]] QVariantList profiles() const;
    [[nodiscard]] QString selectedProfileUuid() const;
    void setSelectedProfileUuid(const QString& uuid);
    [[nodiscard]] QString stateText() const;
    [[nodiscard]] QString lastError() const;
    [[nodiscard]] bool busy() const;
    [[nodiscard]] bool canConnect() const;
    [[nodiscard]] bool canDisconnect() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void connectSelected();
    Q_INVOKABLE void disconnectSelected();
    Q_INVOKABLE void clearError();

  Q_SIGNALS:
    void backendAvailableChanged();
    void profilesChanged();
    void selectedProfileUuidChanged();
    void stateTextChanged();
    void lastErrorChanged();
    void busyChanged();
    void canConnectChanged();
    void canDisconnectChanged();

  private:
    struct DerivedState {
        QString stateText;
        bool busy;
        bool canConnect;
        bool canDisconnect;
    };

    [[nodiscard]] DerivedState derivedState() const;
    void emitDerivedChanges(const DerivedState& previous);
    void handleAvailabilityChanged(bool available);
    void handleProfilesChanged(const VpnProfiles& profiles);
    void handleErrorOccurred(const QString& message);
    [[nodiscard]] const VpnProfile* selectedProfile() const;
    [[nodiscard]] static QString stateToText(VpnState state);

    IVpnBackend& m_backend;
    bool m_backendAvailable;
    VpnProfiles m_profiles;
    QString m_selectedProfileUuid;
    QString m_lastError;
};
