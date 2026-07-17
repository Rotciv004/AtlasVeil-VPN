// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/VpnController.hpp"
#include "fakes/FakeVpnBackend.hpp"

#include <QSignalSpy>
#include <QTest>
#include <QVariantMap>

namespace {

VpnProfile makeProfile(const QString& name, const QString& uuid,
                       VpnState state = VpnState::Disconnected) {
    return VpnProfile{name, uuid, state};
}

} // namespace

class VpnControllerTest final : public QObject {
    Q_OBJECT

  private slots:
    void profilesAreExposedAndFirstProfileIsSelected();
    void refreshDelegatesExactlyOnce();
    void connectDelegatesSelectedUuid();
    void disconnectDelegatesSelectedUuid();
    void connectIsBlockedWhileAnotherProfileIsActive();
    void connectIsBlockedWhileAProfileIsTransitioning();
    void unavailableBackendDisablesActions();
    void unknownSelectedUuidIsRejected();
    void selectionSurvivesProfileReordering();
    void selectionFallsBackWhenSelectedProfileIsRemoved();
    void emptyProfilesClearSelectionAndDisableActions();
    void backendErrorsUpdateLastError();
    void clearErrorRemovesTheError();
    void validActionClearsPreviousError();
    void stateTextAndBusyFollowSelectedProfile();
    void errorStateCanBeDisconnected();
    void propertyNotificationsTrackActualChanges();
    void disconnectIsBlockedWhileDisconnected();
};

void VpnControllerTest::profilesAreExposedAndFirstProfileIsSelected() {
    FakeVpnBackend backend;
    VpnController controller(backend);

    backend.publishProfiles({makeProfile(QStringLiteral("Germany"), QStringLiteral("germany")),
                             makeProfile(QStringLiteral("Romania"), QStringLiteral("romania"))});

    const QVariantList profiles = controller.profiles();
    QCOMPARE(profiles.size(), qsizetype{2});
    QCOMPARE(controller.selectedProfileUuid(), QStringLiteral("germany"));

    const QVariantMap firstProfile = profiles.at(0).toMap();
    QCOMPARE(firstProfile.value(QStringLiteral("name")).toString(), QStringLiteral("Germany"));
    QCOMPARE(firstProfile.value(QStringLiteral("uuid")).toString(), QStringLiteral("germany"));
    QCOMPARE(firstProfile.value(QStringLiteral("state")).toInt(),
             static_cast<int>(VpnState::Disconnected));
    QCOMPARE(firstProfile.value(QStringLiteral("stateText")).toString(),
             QStringLiteral("Disconnected"));
}

void VpnControllerTest::refreshDelegatesExactlyOnce() {
    FakeVpnBackend backend;
    VpnController controller(backend);

    controller.refresh();

    QCOMPARE(backend.refreshCallCount, 1);
}

void VpnControllerTest::connectDelegatesSelectedUuid() {
    FakeVpnBackend backend;
    VpnController controller(backend);
    backend.publishProfiles({makeProfile(QStringLiteral("Germany"), QStringLiteral("germany")),
                             makeProfile(QStringLiteral("Romania"), QStringLiteral("romania"))});
    controller.setSelectedProfileUuid(QStringLiteral("romania"));

    QVERIFY(controller.canConnect());
    controller.connectSelected();

    QCOMPARE(backend.connectCallCount, 1);
    QCOMPARE(backend.lastConnectedUuid, QStringLiteral("romania"));
}

void VpnControllerTest::disconnectDelegatesSelectedUuid() {
    FakeVpnBackend backend;
    VpnController controller(backend);
    backend.publishProfiles(
        {makeProfile(QStringLiteral("Romania"), QStringLiteral("romania"), VpnState::Connected)});

    QVERIFY(controller.canDisconnect());
    controller.disconnectSelected();

    QCOMPARE(backend.disconnectCallCount, 1);
    QCOMPARE(backend.lastDisconnectedUuid, QStringLiteral("romania"));
}

void VpnControllerTest::connectIsBlockedWhileAnotherProfileIsActive() {
    FakeVpnBackend backend;
    VpnController controller(backend);
    backend.publishProfiles(
        {makeProfile(QStringLiteral("Germany"), QStringLiteral("germany")),
         makeProfile(QStringLiteral("Romania"), QStringLiteral("romania"), VpnState::Connected)});

    QVERIFY(!controller.canConnect());
    controller.connectSelected();

    QCOMPARE(backend.connectCallCount, 0);
}

void VpnControllerTest::connectIsBlockedWhileAProfileIsTransitioning() {
    FakeVpnBackend backend;
    VpnController controller(backend);
    const VpnProfile selected = makeProfile(QStringLiteral("Germany"), QStringLiteral("germany"));

    backend.publishProfiles(
        {selected,
         makeProfile(QStringLiteral("Romania"), QStringLiteral("romania"), VpnState::Connecting)});
    QVERIFY(!controller.canConnect());
    QVERIFY(controller.busy());
    controller.connectSelected();
    QCOMPARE(backend.connectCallCount, 0);

    backend.publishProfiles(
        {selected, makeProfile(QStringLiteral("Romania"), QStringLiteral("romania"),
                               VpnState::Disconnecting)});
    QVERIFY(!controller.canConnect());
    QVERIFY(controller.busy());
    controller.connectSelected();
    QCOMPARE(backend.connectCallCount, 0);
}

void VpnControllerTest::unavailableBackendDisablesActions() {
    FakeVpnBackend backend;
    VpnController controller(backend);
    backend.publishProfiles({makeProfile(QStringLiteral("Germany"), QStringLiteral("germany"))});
    backend.publishAvailability(false);

    QVERIFY(!controller.backendAvailable());
    QVERIFY(!controller.canConnect());
    controller.connectSelected();
    QCOMPARE(backend.connectCallCount, 0);

    backend.publishProfiles(
        {makeProfile(QStringLiteral("Germany"), QStringLiteral("germany"), VpnState::Connected)});
    QVERIFY(!controller.canDisconnect());
    controller.disconnectSelected();
    QCOMPARE(backend.disconnectCallCount, 0);
}

void VpnControllerTest::unknownSelectedUuidIsRejected() {
    FakeVpnBackend backend;
    VpnController controller(backend);
    backend.publishProfiles({makeProfile(QStringLiteral("Germany"), QStringLiteral("germany")),
                             makeProfile(QStringLiteral("Romania"), QStringLiteral("romania"))});

    controller.setSelectedProfileUuid(QStringLiteral("not-present"));

    QCOMPARE(controller.selectedProfileUuid(), QStringLiteral("germany"));
}

void VpnControllerTest::selectionSurvivesProfileReordering() {
    FakeVpnBackend backend;
    VpnController controller(backend);
    const VpnProfile germany = makeProfile(QStringLiteral("Germany"), QStringLiteral("germany"));
    const VpnProfile romania = makeProfile(QStringLiteral("Romania"), QStringLiteral("romania"));
    backend.publishProfiles({germany, romania});
    controller.setSelectedProfileUuid(QStringLiteral("romania"));

    backend.publishProfiles({romania, germany});

    QCOMPARE(controller.selectedProfileUuid(), QStringLiteral("romania"));
}

void VpnControllerTest::selectionFallsBackWhenSelectedProfileIsRemoved() {
    FakeVpnBackend backend;
    VpnController controller(backend);
    const VpnProfile germany = makeProfile(QStringLiteral("Germany"), QStringLiteral("germany"));
    const VpnProfile romania = makeProfile(QStringLiteral("Romania"), QStringLiteral("romania"));
    backend.publishProfiles({germany, romania});
    controller.setSelectedProfileUuid(QStringLiteral("romania"));

    backend.publishProfiles({germany});

    QCOMPARE(controller.selectedProfileUuid(), QStringLiteral("germany"));
}

void VpnControllerTest::emptyProfilesClearSelectionAndDisableActions() {
    FakeVpnBackend backend;
    VpnController controller(backend);
    backend.publishProfiles(
        {makeProfile(QStringLiteral("Germany"), QStringLiteral("germany"), VpnState::Connected)});

    backend.publishProfiles({});

    QVERIFY(controller.profiles().isEmpty());
    QVERIFY(controller.selectedProfileUuid().isEmpty());
    QVERIFY(!controller.canConnect());
    QVERIFY(!controller.canDisconnect());
}

void VpnControllerTest::backendErrorsUpdateLastError() {
    FakeVpnBackend backend;
    VpnController controller(backend);

    backend.publishError(QStringLiteral("Authorization was denied."));

    QCOMPARE(controller.lastError(), QStringLiteral("Authorization was denied."));
}

void VpnControllerTest::clearErrorRemovesTheError() {
    FakeVpnBackend backend;
    VpnController controller(backend);
    backend.publishError(QStringLiteral("NetworkManager is unavailable."));

    controller.clearError();

    QVERIFY(controller.lastError().isEmpty());
}

void VpnControllerTest::validActionClearsPreviousError() {
    FakeVpnBackend backend;
    VpnController controller(backend);
    backend.publishProfiles({makeProfile(QStringLiteral("Germany"), QStringLiteral("germany"))});
    backend.publishError(QStringLiteral("Previous error"));

    controller.connectSelected();

    QVERIFY(controller.lastError().isEmpty());
    QCOMPARE(backend.connectCallCount, 1);
}

void VpnControllerTest::stateTextAndBusyFollowSelectedProfile() {
    FakeVpnBackend backend;
    VpnController controller(backend);

    backend.publishProfiles({makeProfile(QStringLiteral("Germany"), QStringLiteral("germany"),
                                         VpnState::Disconnected)});
    QCOMPARE(controller.stateText(), QStringLiteral("Disconnected"));
    QVERIFY(!controller.busy());

    backend.publishProfiles(
        {makeProfile(QStringLiteral("Germany"), QStringLiteral("germany"), VpnState::Connecting)});
    QCOMPARE(controller.stateText(), QStringLiteral("Connecting"));
    QVERIFY(controller.busy());

    backend.publishProfiles(
        {makeProfile(QStringLiteral("Germany"), QStringLiteral("germany"), VpnState::Connected)});
    QCOMPARE(controller.stateText(), QStringLiteral("Connected"));
    QVERIFY(!controller.busy());

    backend.publishProfiles({makeProfile(QStringLiteral("Germany"), QStringLiteral("germany"),
                                         VpnState::Disconnecting)});
    QCOMPARE(controller.stateText(), QStringLiteral("Disconnecting"));
    QVERIFY(controller.busy());

    backend.publishProfiles(
        {makeProfile(QStringLiteral("Germany"), QStringLiteral("germany"), VpnState::Error)});
    QCOMPARE(controller.stateText(), QStringLiteral("Error"));
    QVERIFY(!controller.busy());
}

void VpnControllerTest::errorStateCanBeDisconnected() {
    FakeVpnBackend backend;
    VpnController controller(backend);
    backend.publishProfiles(
        {makeProfile(QStringLiteral("Germany"), QStringLiteral("germany"), VpnState::Error)});

    QVERIFY(controller.canDisconnect());
    controller.disconnectSelected();

    QCOMPARE(backend.disconnectCallCount, 1);
    QCOMPARE(backend.lastDisconnectedUuid, QStringLiteral("germany"));
}

void VpnControllerTest::propertyNotificationsTrackActualChanges() {
    FakeVpnBackend backend;
    VpnController controller(backend);
    QSignalSpy availabilitySpy(&controller, &VpnController::backendAvailableChanged);
    QSignalSpy profilesSpy(&controller, &VpnController::profilesChanged);
    QSignalSpy selectionSpy(&controller, &VpnController::selectedProfileUuidChanged);
    QSignalSpy stateTextSpy(&controller, &VpnController::stateTextChanged);
    QSignalSpy busySpy(&controller, &VpnController::busyChanged);
    QSignalSpy canConnectSpy(&controller, &VpnController::canConnectChanged);
    QSignalSpy canDisconnectSpy(&controller, &VpnController::canDisconnectChanged);
    const VpnProfile disconnected =
        makeProfile(QStringLiteral("Germany"), QStringLiteral("germany"));

    backend.publishProfiles({disconnected});
    QCOMPARE(profilesSpy.count(), 1);
    QCOMPARE(selectionSpy.count(), 1);
    QCOMPARE(stateTextSpy.count(), 1);
    QCOMPARE(busySpy.count(), 0);
    QCOMPARE(canConnectSpy.count(), 1);
    QCOMPARE(canDisconnectSpy.count(), 0);

    backend.publishProfiles({disconnected});
    controller.setSelectedProfileUuid(QStringLiteral("unknown"));
    QCOMPARE(profilesSpy.count(), 1);
    QCOMPARE(selectionSpy.count(), 1);
    QCOMPARE(stateTextSpy.count(), 1);
    QCOMPARE(canConnectSpy.count(), 1);

    backend.publishProfiles(
        {makeProfile(QStringLiteral("Germany"), QStringLiteral("germany"), VpnState::Connecting)});
    QCOMPARE(profilesSpy.count(), 2);
    QCOMPARE(stateTextSpy.count(), 2);
    QCOMPARE(busySpy.count(), 1);
    QCOMPARE(canConnectSpy.count(), 2);

    backend.publishProfiles(
        {makeProfile(QStringLiteral("Germany"), QStringLiteral("germany"), VpnState::Connected)});
    QCOMPARE(profilesSpy.count(), 3);
    QCOMPARE(stateTextSpy.count(), 3);
    QCOMPARE(busySpy.count(), 2);
    QCOMPARE(canConnectSpy.count(), 2);
    QCOMPARE(canDisconnectSpy.count(), 1);

    backend.publishAvailability(false);
    backend.publishAvailability(false);
    QCOMPARE(availabilitySpy.count(), 1);
    QCOMPARE(canDisconnectSpy.count(), 2);
}

void VpnControllerTest::disconnectIsBlockedWhileDisconnected() {
    FakeVpnBackend backend;
    VpnController controller(backend);
    backend.publishProfiles({makeProfile(QStringLiteral("Germany"), QStringLiteral("germany"))});

    QVERIFY(!controller.canDisconnect());
    controller.disconnectSelected();

    QCOMPARE(backend.disconnectCallCount, 0);
}

QTEST_GUILESS_MAIN(VpnControllerTest)

#include "tst_vpncontroller.moc"
