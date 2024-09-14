#!/bin/sh

# Check if the Marvell station driver ever failed to load.
# If it did, reboot the host.

if ! journalctl -n -k --no-pager > /dev/null ; then
    echo "Unable to get system logs."
    echo "You either don't have permissions to read system logs, or you are not using systemd."
    exit 1
fi

if journalctl -k -b | grep -q -E 'wlan_pcie: probe of .* failed with error -14' ; then
    echo "The Marvell station driver failed to load properly, rebooting."
    reboot
fi
