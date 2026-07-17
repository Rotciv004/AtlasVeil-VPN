// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root

    // The context property is the intentional boundary between QML and the controller.
    // qmllint disable unqualified
    readonly property var controller: vpnController
    // qmllint enable unqualified

    width: 480
    height: 390
    minimumWidth: 400
    minimumHeight: 340
    visible: true
    title: "AtlasVeil"

    pageStack.initialPage: Kirigami.Page {
        title: "AtlasVeil"
        padding: Kirigami.Units.largeSpacing

        ColumnLayout {
            anchors.fill: parent
            spacing: Kirigami.Units.largeSpacing

            Controls.Label {
                Layout.fillWidth: true
                text: "AtlasVeil"
                font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.6
                font.weight: Font.DemiBold
            }

            Controls.Label {
                Layout.fillWidth: true
                text: "Controls existing WireGuard profiles managed by NetworkManager."
                wrapMode: Text.WordWrap
            }

            Kirigami.InlineMessage {
                Layout.fillWidth: true
                visible: !root.controller.backendAvailable
                type: Kirigami.MessageType.Error
                text: "NetworkManager is unavailable."
            }

            Kirigami.InlineMessage {
                id: errorMessage

                Layout.fillWidth: true
                visible: false
                type: Kirigami.MessageType.Error
                text: root.controller.lastError
                showCloseButton: true

                onVisibleChanged: {
                    if (!visible && root.controller.lastError.length > 0) {
                        root.controller.clearError()
                    }
                }

                Connections {
                    target: root.controller

                    function onLastErrorChanged() {
                        errorMessage.visible = root.controller.lastError.length > 0
                    }
                }

                Component.onCompleted: visible = root.controller.lastError.length > 0
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                Controls.Label {
                    text: "WireGuard profile"
                    font.weight: Font.DemiBold
                }

                Controls.ComboBox {
                    id: profileSelector

                    Layout.fillWidth: true
                    model: root.controller.profiles
                    textRole: "name"
                    valueRole: "uuid"
                    enabled: count > 0 && !root.controller.busy
                    Accessible.name: "WireGuard profile"

                    function synchronizeIndex() {
                        currentIndex = indexOfValue(root.controller.selectedProfileUuid)
                    }

                    onActivated: root.controller.selectedProfileUuid = currentValue
                    onModelChanged: Qt.callLater(synchronizeIndex)
                    Component.onCompleted: synchronizeIndex()

                    Connections {
                        target: root.controller

                        function onProfilesChanged() {
                            Qt.callLater(profileSelector.synchronizeIndex)
                        }

                        function onSelectedProfileUuidChanged() {
                            profileSelector.synchronizeIndex()
                        }
                    }
                }

                Controls.Label {
                    Layout.fillWidth: true
                    visible: profileSelector.count === 0
                    text: "No WireGuard profiles found"
                    wrapMode: Text.WordWrap
                    color: Kirigami.Theme.disabledTextColor
                }
            }

            RowLayout {
                Layout.fillWidth: true
                visible: profileSelector.count > 0
                spacing: Kirigami.Units.smallSpacing

                Controls.Label {
                    Layout.fillWidth: true
                    text: "Status: " + root.controller.stateText
                }

                Controls.BusyIndicator {
                    visible: root.controller.busy
                    running: visible
                    Accessible.name: root.controller.stateText
                }
            }

            Item {
                Layout.fillHeight: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                Controls.Button {
                    text: "Refresh"
                    icon.name: "view-refresh"
                    enabled: !root.controller.busy
                    onClicked: root.controller.refresh()
                }

                Item {
                    Layout.fillWidth: true
                }

                Controls.Button {
                    text: root.controller.canDisconnect ? "Disconnect" : "Connect"
                    icon.name: root.controller.canDisconnect
                        ? "network-disconnect"
                        : "network-connect"
                    highlighted: true
                    enabled: root.controller.canConnect || root.controller.canDisconnect

                    onClicked: {
                        if (root.controller.canDisconnect) {
                            root.controller.disconnectSelected()
                        } else {
                            root.controller.connectSelected()
                        }
                    }
                }
            }
        }
    }
}
