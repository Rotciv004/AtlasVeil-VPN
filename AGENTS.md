# AtlasVeil Repository Guidance

## Architecture

Keep the production dependency flow one-way:

```text
QML/Kirigami UI -> VpnController -> IVpnBackend -> NetworkManagerAdapter
                 -> NetworkManagerQt -> NetworkManager over D-Bus -> WireGuard
```

QML owns presentation only. `VpnController` owns UI-facing state and action rules.
`IVpnBackend` is the test boundary, and `NetworkManagerAdapter` is the only production class that
may access NetworkManagerQt.

## Build, test, and format

Run commands from the repository root:

```bash
cmake --preset dev
cmake --build --preset dev --parallel
ctest --preset dev
clang-format -i src/main.cpp src/core/*.cpp src/core/*.hpp src/network/*.cpp \
    src/network/*.hpp tests/fakes/*.hpp tests/unit/*.cpp
```

Do not commit generated build output or local IDE state.

## C++ and QML conventions

Use C++23, four spaces, no tabs, attached braces, a 100-column limit, and left-aligned pointers
and references. Keep include blocks sorted. Add `// SPDX-License-Identifier: GPL-3.0-or-later` to
C++ and QML sources. Use English for source identifiers, UI text, and errors. Keep QML declarative
and avoid duplicating controller business rules in JavaScript.

Do not add speculative data fields or abstractions. In particular, do not model or expose private
keys, credentials, endpoints, configuration paths, DNS, public IP, latency, or traffic unless a
future, reviewed requirement explicitly changes the product scope.

## Security boundaries

Never retrieve, display, copy, export, or log NetworkManager secrets or private keys. Application
code must not execute `nmcli`, `wg`, `wg-quick`, shell networking commands, or privileged helpers.
Do not use `QProcess`, `system`, `popen`, or direct reads of NetworkManager connection files. Run
the GUI as the normal desktop user and leave authorization to NetworkManager and PolicyKit.

Unit tests must use an `IVpnBackend` fake. They must not contact D-Bus, activate live connections,
or alter routes, DNS, firewall rules, NetworkManager profiles, or WireGuard interfaces. Live
network checks belong only in the documented manual integration checklist on a disposable local
machine.

## Definition of done

A change is complete when the dev preset configures, the project builds, all unit tests pass,
format and static checks are clean, documentation matches actual behavior, and a final diff review
finds no generated output, credentials, prohibited networking subprocesses, secret retrieval,
misleading security claims, or unrelated changes.
