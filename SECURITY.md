# Security Policy

## Reporting a vulnerability

Report suspected vulnerabilities privately through this repository's **Security > Report a
vulnerability** workflow on GitHub. If private vulnerability reporting is unavailable, contact the
repository owner through a private channel and ask for a secure reporting method before sharing
technical details.

Do not open a public issue containing credentials, WireGuard or VPN profiles, private keys,
preshared keys, access tokens, complete NetworkManager settings, endpoint details that must remain
private, or unsanitized logs. A useful private report includes the affected version, operating
system, reproducible steps, expected and observed behavior, impact, and a minimal sanitized example.
Remove all secret and personally identifying material first.

## Security boundary

AtlasVeil 0.1.0 is a normal-user desktop controller for existing NetworkManager WireGuard profiles.
Its trust boundary is intentionally narrow:

- QML calls `VpnController`, which calls `IVpnBackend`.
- The production `NetworkManagerAdapter` uses NetworkManagerQt over D-Bus.
- NetworkManager and PolicyKit own authorization and privileged network changes.
- NetworkManager, the WireGuard implementation, the saved profile, the operating system, and the
  remote endpoint remain outside AtlasVeil's security boundary.

AtlasVeil must not be run as a root GUI. It does not install a privileged helper or background
daemon. It does not execute networking shell commands and does not read NetworkManager connection
files directly.

## Credentials and profiles

AtlasVeil does not request NetworkManager secrets and must never read, display, copy, export, or log
private keys, preshared keys, passwords, or complete connection settings. It retains only the saved
profile's human-readable name, NetworkManager UUID, and connection state for UI control.

Treat WireGuard configuration files and NetworkManager profiles as credentials. Store, transmit,
and report them accordingly. Profile import and editing are outside the 0.1.0 scope.

## Known limitations in 0.1.0

A NetworkManager `Connected` state means only that the profile is active. It does not verify the
remote endpoint, public IP, routes, name resolution, traffic confidentiality, or leak resistance.
AtlasVeil 0.1.0 provides no:

- kill switch or firewall enforcement;
- DNS configuration or DNS-leak guarantee;
- IPv6 configuration or IPv6-leak guarantee;
- route-integrity or split-tunneling enforcement;
- public-IP, endpoint-identity, latency, or traffic verification; or
- automatic reconnection or fail-closed behavior.

Traffic may continue over another interface before activation, after deactivation, or when a VPN
operation fails. AtlasVeil is therefore not a complete privacy or leak-proof VPN product.

## Security-sensitive contributions

Changes involving routes, DNS, firewall policy, IPv6, credentials, profile import/export, secret
storage, privilege boundaries, or authorization require all of the following before acceptance:

1. a documented threat model and explicit non-goals;
2. review of privilege, rollback, cancellation, and fail-open/fail-closed behavior;
3. proof that secrets cannot enter UI state, logs, errors, crash reports, or tests;
4. deterministic unit tests that do not touch the live network; and
5. a safe manual integration plan for a disposable local machine with console access.

Never run route-changing or VPN integration tests on a remote system whose access depends on the
route being tested.
