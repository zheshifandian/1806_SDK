#!usr/bin/lua

local uci = require "luci.model.uci"
local _uci_real  = cursor or _uci_real or uci.cursor()
local sysutil = require "luci.siwifi.sf_sysutil"
local nw = require "luci.model.network"
local luci_sys = require "luci.sys"

function recovery_wifi_status()
	_uci_real:foreach("wireless","wifi-iface",
	function(s)
		if s.ifname ~= "sfi0" and s.ifname ~= "sfi1" then
			wifi_status = _uci_real:get("wireless", s[".name"], "tmp_status") or 0
			_uci_real:set("wireless", s[".name"], "disabled", wifi_status)
			_uci_real:delete("wireless", s[".name"], "tmp_status")
		end
	end)
	_uci_real:save("wireless")
	_uci_real:commit("wireless")
end

function wds_disable()
	_uci_real:foreach("firewall","zone",
	function(s)
		if(s["name"] == "lan") then
			_uci_real:set("firewall", s[".name"], "network", "lan")
		end
	end)
	_uci_real:set("firewall","@defaults[0]","flow_offloading",1)
	_uci_real:set("firewall","@defaults[0]","flow_offloading_hw",1)
	_uci_real:save("firewall")
	_uci_real:commit("firewall")

	_uci_real:delete("dhcp", "lan", "ignore")
	_uci_real:save("dhcp")
	_uci_real:commit("dhcp")

	nw:del_network("stabridge")
	nw:del_network("wwan")
	_uci_real:delete("network", "wan", "disabled")
	_uci_real:save("network")
	_uci_real:commit("network")
	luci_sys.call("/etc/init.d/network restart")

	nw:del_wifinet("sfi0")
	nw:del_wifinet("sfi1")
	recovery_wifi_status()
end

-- use 30s to check wds_enable if finish
finish = "0"
for i = 30, 1, -1 do
	sync_file = io.open("/tmp/wds_sync", "r")
	if sync_file ~= nil then
		io.input("/tmp/wds_sync")
		finish = io.read(1)
		io.close(sync_file)
	end
	if finish == "1" then
		break
	end
	luci_sys.call("sleep 1")
end

if finish == "1" then
-- check wds_enable finish, wds_sync == 1
	luci_sys.call("/etc/init.d/network restart")
	--wireless interface recovery
	recovery_wifi_status()

	sysutil.fork_exec("sleep 1; /sbin/check_wds")
else
	-- wds_enable wds killed by somewhere else and wds_sync == 0
	-- in this situation, we need recovery all settings
	sync_file = io.open("/tmp/wds_sync", "w+")
	if sync_file ~= nil then
		sync_file:write(1)
		sync_file:flush()
		io.close(sync_file)
	end
	wds_disable()
end
