#!/bin/sh

. /lib/functions.sh

load_firmware()
{
	cmd="/usr/sbin/insmod startcore"
	eval $cmd
}

unload_firmware()
{
	cmd="/usr/sbin/rmmod startcore"
	eval $cmd
}

insmod_rf() {
#	umount /sys/kernel/debug
#	mount -t debugfs none /sys/kernel/debug
	modprobe sf16a18_rf
	sleep 1
}

unload_rf() {
	cmd="/usr/sbin/rmmod sf16a18_rf"
	eval $cmd
	sleep 1
}

insmod_umac(){
	modparams="
		agg_tx=${agg_tx-1}
		amsdu_force=${amsdu_force-2}
		ap_uapsd_on=${ap_uapsd_on-0}
		autobcn=${autobcn-1}
		bwsig_on=${bwsig_on-0}
		cmon=${cmon-1}
		custregd=${custregd-1}
		dpsm=${dpsm-1}
		dynbw_on=${dynbw_on-0}
		gf_on=${gf_on-0}
		hwscan=${hwscan-1}
		phycfg=${phycfg-0}
		ps_on=${ps_on-1}
		sgi=${sgi-1}
		uapsd_timeout=${uapsd_timeout-0}
		use_2040=${use_2040-1}
		vht_on=${vht_on-1}
		dyndbg=${dyndbg-}
		use_80=${use_80-1}
		ht_on=${ht_on-1}
		tx_lft=${tx_lft-100}
		nss=${nss-1}
		ldpc_on=${ldpc_on-1}
	"
	cmd="/usr/sbin/insmod $1 $modparams"
	eval $cmd
}

insmod_mac80211(){
	modprobe cfg80211
	modprobe mac80211
}

insmod_umac_lb(){
	insmod_umac sf16a18_lb_smac
}

insmod_umac_hb(){
	insmod_umac sf16a18_hb_smac
}

insmod_smac() {
	insmod_umac_lb
	insmod_umac_hb
}

unload_umac_lb(){
	cmd="/usr/sbin/rmmod sf16a18_lb_smac"
	eval $cmd
}

unload_umac_hb(){
	cmd="/usr/sbin/rmmod sf16a18_hb_smac"
	eval $cmd
}

unload_smac(){
	unload_umac_lb
	unload_umac_hb
}
