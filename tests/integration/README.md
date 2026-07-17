# Manual integration testing

Live VPN activation is intentionally not automated. Activation changes host networking, can
replace routes or DNS behavior according to the saved profile, may display a PolicyKit prompt, and
depends on external endpoints and machine-specific NetworkManager state. An automated test could
disconnect its own runner or the person administering it. Controller unit tests therefore use a
fake `IVpnBackend` and never contact NetworkManager or D-Bus.

Perform this checklist only on a disposable local Fedora machine or virtual machine with direct
console access. Do not perform route-changing tests on a remote machine when SSH, remote desktop,
or other access may depend on the route under test. Use non-production test profiles and remove
them manually after testing if appropriate.

## Preparation

- Build and run AtlasVeil as a normal desktop user.
- Confirm NetworkManager is running and PolicyKit prompts can be observed locally.
- Prepare up to two disposable WireGuard profiles with distinct names and UUIDs.
- Keep profile contents, keys, and unsanitized logs out of test reports.

## Checklist

### No profiles

1. Ensure the test machine has no saved WireGuard profiles.
2. Start AtlasVeil or press **Refresh**.
3. Confirm it shows **No WireGuard profiles found**, clears selection, and disables the primary
   action.

### One valid profile

1. Add one valid disposable WireGuard profile outside AtlasVeil.
2. Confirm its human-readable name appears and becomes selected.
3. Confirm the initial status matches NetworkManager and **Connect** is enabled while disconnected.

### Connect, disconnect, and reconnect

1. Connect the valid profile and confirm a busy/connecting state appears without freezing the UI.
2. Confirm AtlasVeil eventually reports **Connected** when NetworkManager activates the profile.
3. Disconnect it and confirm the disconnecting state followed by **Disconnected**.
4. Connect and disconnect it once more to confirm a completed operation does not block reuse.
5. Change the connection state through Plasma while AtlasVeil remains open and confirm the UI
   follows the external change.

### Invalid endpoint

1. Use a disposable profile whose endpoint is deliberately invalid or unreachable.
2. Request connection and confirm AtlasVeil stays responsive.
3. Confirm the final state/error follows NetworkManager and any error shown is readable and does
   not expose keys, complete settings, or raw configuration contents.

### Denied PolicyKit authorization

1. Use a policy/test account for which NetworkManager activation requires authorization.
2. Request connection and deny or cancel the PolicyKit prompt.
3. Confirm the request fails cleanly, the busy state clears, and the message contains no secrets.

### NetworkManager restart

1. With AtlasVeil open, restart NetworkManager using the test machine's normal administration
   process.
2. Confirm AtlasVeil reports backend unavailability while NetworkManager is unavailable.
3. Confirm profiles and current state return after NetworkManager is available again and **Refresh**
   remains usable when no operation is pending.

### Two profiles require an explicit disconnect

1. Save two distinct disposable WireGuard profiles.
2. Connect the first profile.
3. Confirm the second profile cannot be activated while the first is active, connecting, or
   disconnecting.
4. Explicitly disconnect the first profile.
5. Select and connect the second profile only after every WireGuard profile is disconnected.

Record only sanitized observations. Never attach profile files, private keys, preshared keys, or
complete NetworkManager settings to a test report.
