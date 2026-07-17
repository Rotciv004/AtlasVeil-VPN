# AtlasVeil

AtlasVeil 0.1.0 is an open-source Linux desktop controller for existing WireGuard connection
profiles managed by NetworkManager. It provides a small, native C++23, Qt 6, and KDE Kirigami
interface for selecting, connecting, and disconnecting those profiles on a Fedora KDE Plasma
desktop.

AtlasVeil is not a VPN protocol, VPN provider, server-provisioning service, or complete privacy
product. It does not create profiles, choose real servers by country, or guarantee that traffic is
leak-proof. A profile named `Germany`, for example, is only a user-provided name; AtlasVeil neither
creates nor verifies a server in Germany. A `Connected` status means only that NetworkManager
reports the selected profile as active.

## Version 0.1.0 scope

AtlasVeil can:

- discover saved NetworkManager profiles whose connection type is WireGuard;
- display profile names while retaining NetworkManager UUIDs as stable identifiers;
- show backend availability and the selected profile's state;
- request asynchronous activation or deactivation through NetworkManager;
- prevent activation of a second WireGuard profile while another is active or transitioning;
- reflect changes made externally through Plasma or `nmcli`;
- show readable errors and an explicit empty state when no WireGuard profiles exist; and
- run as the normal desktop user, with authorization delegated to NetworkManager and PolicyKit.

Version 0.1.0 does not import or edit profiles, generate keys, retrieve secrets, manage VPN
servers, switch countries automatically, reconnect automatically, or provide a kill switch. It
does not configure DNS, firewalls, IPv6 protection, split tunneling, or leak protection. It also
does not perform public-IP checks, latency measurements, traffic accounting, telemetry, account
management, or provider API calls. There is no background daemon and no Flatpak, mobile, or
Windows package in this release.

## Architecture

```text
QML/Kirigami UI
    -> VpnController
    -> IVpnBackend
    -> NetworkManagerAdapter
    -> KDE Frameworks NetworkManagerQt
    -> NetworkManager over D-Bus
    -> Linux WireGuard support
```

QML contains presentation logic only. `VpnController` owns UI-facing state and action rules.
`IVpnBackend` isolates the controller from the operating system, allowing unit tests to use a fake
without touching the live network. `NetworkManagerAdapter` is the only production class that uses
NetworkManagerQt. AtlasVeil does not execute `nmcli`, `wg`, `wg-quick`, or shell commands.

## Fedora dependencies

Install the build and runtime development dependencies on Fedora:

```bash
sudo dnf install gcc-c++ cmake ninja-build extra-cmake-modules \
    qt6-qtbase-devel qt6-qtdeclarative-devel kf6-kirigami-devel \
    kf6-qqc2-desktop-style kf6-networkmanager-qt-devel
```

On Fedora 44, `qt6-qtdeclarative-devel` provides the Qt Quick Controls 2 development files, and
the Kirigami development package is named `kf6-kirigami-devel`.

NetworkManager and Linux WireGuard support must already be available on the system.
`wireguard-tools` is optional for manual administration and is never invoked by AtlasVeil.

## Configure, build, and test

From the repository root:

```bash
cmake --preset dev
cmake --build --preset dev --parallel
ctest --preset dev
```

The dev preset creates `build/dev/compile_commands.json` for clangd. To create a release build:

```bash
cmake --preset release
cmake --build --preset release --parallel
```

## Run and install

Run the development build as the normal desktop user:

```bash
./build/dev/atlasveil
```

For an optional per-user installation:

```bash
cmake --install build/release --prefix "$HOME/.local"
```

Do not run the application as root. When an operation needs authorization, NetworkManager asks
PolicyKit to apply the system's policy and may present the desktop authorization dialog.

## Add a WireGuard profile first

Importing a profile is an explicit user administration task and is never performed by AtlasVeil or
its automated tests. Obtain a trusted WireGuard configuration from your administrator or provider,
protect it as credential material, and use one of these manual methods:

- In Plasma, open **System Settings > Wi-Fi & Networking > Connections**, add or import a
  WireGuard connection, review it, and save it.
- From a terminal, deliberately run
  `nmcli connection import type wireguard file /path/to/profile.conf`, then review the saved
  connection. This command is for the user only; AtlasVeil never runs it.

After the profile is saved, use **Refresh** in AtlasVeil if it does not appear immediately.

## Security limitations

AtlasVeil deliberately does not retrieve NetworkManager secrets or display private keys. It sends
profile activation and deactivation requests to NetworkManager and relies on NetworkManager,
PolicyKit, the saved profile, kernel networking, and the remote endpoint for the resulting network
behavior.

Version 0.1.0 has no kill switch and offers no DNS-leak, IPv6-leak, route-integrity, endpoint-
identity, or traffic-confidentiality guarantee of its own. Traffic can use an unintended route
before connection, after disconnection, or when activation fails. Review the saved profile and the
system's effective routes and DNS configuration independently when those guarantees matter. See
[SECURITY.md](SECURITY.md) for reporting guidance and the full security boundary.

## Recommended future development order

1. Harden state synchronization and error handling against more NetworkManager lifecycle cases.
2. Expand deterministic controller tests and repeat the manual disposable-machine matrix.
3. Improve accessibility, localization readiness, diagnostics, and distribution packaging.
4. Design any profile import or editing feature with an explicit credential-handling threat model.
5. Consider route, DNS, IPv6, firewall, or kill-switch features only after a dedicated threat
   model, failure-mode design, privilege review, and safe integration-test plan exist.

AtlasVeil is licensed under the GNU General Public License, version 3 or later. See
[LICENSE](LICENSE).
