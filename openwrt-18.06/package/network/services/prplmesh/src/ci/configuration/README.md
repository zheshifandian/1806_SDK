# Device configuration files

This directory contains configuration files for the devices used in CI.
For simplicity and flexibility, each file is a shell file that is copied to the device over the serial line, then executed.

## Filename convention

Each filename (without the extension) should also be the name used to refer to the device over SSH and serial.
For example, the Boardfarm device accessible over SSH with the name `turris-omnia-1` and with a serial device under `/dev/turris-omnia-1` will have its configuration file in: `boardfarm/turris-omnia-1.sh`.
