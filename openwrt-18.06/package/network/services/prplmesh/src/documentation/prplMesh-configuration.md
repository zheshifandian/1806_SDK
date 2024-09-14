# Introduction

prplMesh can be configured through different options.
Depending on the platform, those options are set in different ways (see [Platform-specific files](#Platform-specific-files)).

For most platforms, changes in options are only taken into account after prplMesh is restarted.

To change settings at runtime without restarting prplMesh, see also [Configuration via NBAPI](https://gitlab.com/prpl-foundation/prplmesh/prplMesh/-/wikis/prplMesh-Northbound-API#configuration-via-nbapi)

## All options

For each platform, a default configuration file is provided.

Some optional options are not included in default configuration files, and instead have default values within prplMesh.
When a platform-specific configuration file specifies a value for a option, it overrides the default value used by prplMesh.

The tables in the sections below outlines all the existing options, across all platforms.

### Global configuration options

In the following table, "default" means default value used within prplMesh if it's not provided by the platform.

| Option                                       | Type    | Required | Default         | Description                                                                                                                                           |
|----------------------------------------------|---------|----------|-----------------|-------------------------------------------------------------------------------------------------------------------------------------------------------|
| management_mode                              | string  | yes      | *none*          | Multi-AP mode (Agent, Controller, Controller+Agent).                                                                                                  |
| operating_mode                               | string  | yes      | *none*          | Legacy option used together with management_mode `Not-Multi-AP.`, will be removed in the future.                                                      |
| onboarding                                   | bool    | no       | *none*          | The current prplMesh onboarding state. prplMesh doesn't currently set it.                                                                             |
| rdkb_extensions                              | bool    | no       | 0               | If set to 1, enable the rdk-b extensions. Requires BEEROCKS_RDKB to be set at compile-time.                                                           |
| band_steering                                | bool    | no       | 0               | If set to 1, enable client band steering (used by the optimal path feature).                                                                          |
| client_roaming                               | bool    | no       | 0               | If set to 1, enable client client roaming (used by the optimal path feature).                                                                         |
| roaming_hysteresis_percent_bonus             | integer | no       | 10              | Roaming hysteresis bonus percentage (see optimal path feature).                                                                                       |
| dfs_reentry                                  | bool    | no       | 1               | If set to 1, enable DFS re-entry (see channel selection task).                                                                                        |
| best_channel_rank_th                         | integer | no       | 0               | Threshold for the best channel rank (see channel selection task).                                                                                     |
| mem_only_psk                                 | bool    | yes      | *none*          | If set to 1, store the PSK in-memory (see hostapd's 'mem_only_psk' option).                                                                           |
| backhaul_band                                | bool    | yes      | *none*          | The preferred band for wireless backhaul. Supported values: '2.4GHz', '5GHz', 'auto'.                                                                 |
| backhaul_wire_iface                          | string  | yes      | *none*          | The network interface to use for wired backhaul.                                                                                                      |
| certification_mode                           | bool    | yes      | *none*          | If set to 1, enable certification-specific features or behavior (e.g. enable UCC listener).                                                           |
| stop_on_failure_attempts                     | bool    | no       | 1               | If a problem occurs, the number of times to retry before the agent is stopped completely. Set to 0 to disable.                                        |
| persistent_db                                | bool    | no       | 0               | If set to 1, enable the persistent database (stores clients in a database). Requires a platform that supports it.                                     |
| clients_persistent_db_max_size               | integer | no       | 256             | Maximum number of clients to store in the persistent DB.                                                                                              |
| max_timelife_delay_minutes                   | integer | no       | 525600 (1 year) | Maximum lifetime (in minutes) of clients in the persistent DB.                                                                                        |
| unfriendly_device_max_timelife_delay_minutes | integer | no       | 1440 (1 day)    | Maximum lifetime (in minutes) of "unfriendly" clients in the persistent DB.                                                                           |
| persistent_db_aging_interval_sec             | integer | no       | 3600 (1 hour)   | Interval (in seconds) to run the persistent DB aging mechanism.                                                                                       |
| zwdfs_enable                                 | bool    | no       | 0               | If set to 1, enable the "zero wait" DFS feature.                                                                                                      |
| steering_disassoc_timer_msec                 | integer | no       | 200             | Client steering disassociation timer.                                                                                                                 |
| link_metrics_request_interval_sec            | integer | no       | 60              | Interval for periodic link metrics requests from all agents (set to 0 to disable).                                                                    |
| clients_measurement_mode                     | integer | no       | 1               | Client measurements mode. `0` disables, `1` enables the measurements for all clients.`2` enables measurements only for clients selected for steering. |
| mandatory_interfaces                         | string  | no       | *empty*         | Comma-separated list of wireless interfaces that prplMesh should use. If empty, try to use all of them.                                               |
| unsuccessful_assoc_report_policy                         | bool  | no       | 1         | If set to 1, enable reporting for unsuccessful associations.                                               |
| unsuccessful_assoc_max_reporting_rate                         | int  | no       | 30         | Maximum rate for reporting unsuccessful association in attempts per minute.                                               |

### Radio-specific configuration options (UCI only).

In addition to the global configuration options, prplMesh needs to have one configuration section per radio.
The available options of the radio sections are described in the table that follows.

Again, "default" means default value used within prplMesh if it's not provided by the platform.

| Option                  | Type   | Required | Default | Description                                                                                                                        |
|-------------------------|--------|----------|---------|------------------------------------------------------------------------------------------------------------------------------------|
| hostap_iface            | string | yes      | *none*  | One of the (AP) interface name of the radio. For MaxLinear devices, this must be the "radio interface" (i.e. the dummy interface). |
| sta_iface               | string | yes      | *none*  | The station interface to use for this radio.                                                                                       |
| hostap_iface_steer_vaps | string | yes      | *none*  | The interfaces (VAP) that the client can be steered to. If empty, clients can be steered to any VAP (see optimal path task).       |

## Platform-specific files

### Linux builds (dummy)

For the "dummy" variant, the prplMesh options are set in the `prplmesh_platform_db` file.

This file is composed of key and values, separated by an equal sign (`=`).
Trailing whitespace and whitespace at the beginning of a line is removed.
When a number sign (`#`) is encountered, everything that follows until the end of the line is ignored (it can thus be used to include comments in the file).

A default version of the file is included in this repository: `framework/platform/bpl/platform_db/prplmesh_platform_db`).

The following variables are specific to Linux builds:
| Option                                    | Type   | Required | Default | Description                                   |
|-------------------------------------------|--------|----------|---------|-----------------------------------------------|
| hostapd_ctrl_path_<interface_name>        | string | yes      | *none*  | Path to the hostapd's control sockets.        |
| wpa_supplicant_ctrl_path_<interface_name> | string | yes      | *none*  | Path to the wpa_supplicant's control sockets. |

### RDK-B builds

For RDK-B, a `prplmesh_db` file is used.

Unlike for dummy builds, this file is configured at build time using CMAKE options.

The default version is in `framework/platform/bpl/db/uci/prplmesh_db.in`.

The format of this file is the same format as for OpenWrt's UCI options (see prplWrt builds below).

### prplWrt builds

For prplWrt, the [standard UCI system](https://openwrt.org/docs/guide-user/base-system/uci) is used.

A default configuration file also exists, and it's stored [alongside the prplMesh package in the prpl feed](https://gitlab.com/prpl-foundation/prplwrt/feed-prpl/-/blob/prplwrt/prplmesh/files/etc/uci-defaults/prplmesh).
