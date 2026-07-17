// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "VpnState.hpp"

#include <QList>
#include <QString>

struct VpnProfile {
    QString name;
    QString uuid;
    VpnState state{VpnState::Disconnected};

    bool operator==(const VpnProfile&) const = default;
};

using VpnProfiles = QList<VpnProfile>;
