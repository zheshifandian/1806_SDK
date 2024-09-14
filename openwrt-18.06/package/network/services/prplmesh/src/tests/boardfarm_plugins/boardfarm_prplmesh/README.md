Configuration file documentation:
=================================

Configuration is stored in JSON format in following layout:

```
{
    <setup name>: {
        <list of setup properties>
        "devices": [
            {
                <list of device properties>
            },
            ...
        ]
    },
    <other setup>: {
        ...
    },
    ...
}
```

Properties related to both setup and device:
--------------------------------------------

* **name**: just a name. Used for naming log directories.
* **role**: prplMesh role: either "agent" or "controller".
* **delay**: time in seconds given to the device to initialize.
* **conn\_cmd**: command to connect to the device console.
	* Empty if current console is the remote console (typically for docker devices).
	* For real devices:
		* in practice:  this parameter is ignored. It has just to be present.
		  SSH connection to **control_ip** is used.
		* in theory: It is meant to be a command to connect via serial port/ssh/etc.
		  Example: `cu -s 115200 -l /dev/ttyUSB0 -f`.
* **docker\_network**: docker network name.

Setup properties:
-----------------
* **board\_type**: board type. It has to match the model of the python class.
  Boardfarm has functionality to run test on a board of given type (`bft -b <board_type>`).
* **iface\_to\_device**: interface on which prplMesh device is connected.
	This interface is added to the bridge where all boardfarm devices communicate.
	Currently only one interface is supported.
	If you need more than one you can either modify the source code (the change is trivial)
	or connect all your devices via an intermediate switch.
* **control_ip**: IP address of the connected prplMesh device on control network. This is used to connect to the device using SSH.
* **host_ip_to_device**: IP address of the boardfarm host device. This is used so the bridge can communicate with other network to ensure wlan/wan requests don't get mixed.

Device properties:
------------------
* **iface**: Used for STAs. Interface to device under test. Default value is "wlan0".
* **driver**: hostapd driver name(s). Comma-separated list.
  Default for real stations: "nl80211,wext".
* **connection\_type**: connection type.
  One of the following: "ser2net", "local\_serial", "ssh", "local\_cmd".
  If none specified, warning is issued by boardfarm and "ser2net" is used.
  On agents (both real and dummy): not used and not required.
* **color**: color of console output for given device.
* **station_ip**: IP address of the connected device on control network. This is used to connect to the device using SSH.
* **station_pw**: Device password for a ssh connection
* **station_ip_wifi**: IP address of the connected wifi device network. This is used to bind test data traffic.
