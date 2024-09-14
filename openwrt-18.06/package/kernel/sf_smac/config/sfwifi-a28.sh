#!/bin/sh

. /lib/functions.sh

INSMOD="/sbin/insmod"
RMMOD="/sbin/rmmod"

load_firmware()
{
    cmd="/sbin/insmod startcore"
    eval $cmd
}

unload_firmware()
{
    cmd="/sbin/rmmod startcore"
    eval $cmd
}

insmod_rf() {
#   umount /sys/kernel/debug
#   mount -t debugfs none /sys/kernel/debug
    modprobe sf16a18_rf
    sleep 1
}

insmod_rf_ate() {
#   umount /sys/kernel/debug
#   mount -t debugfs none /sys/kernel/debug
    cmd="/sbin/insmod sf16a18_rf thermal_on=1"
	eval $cmd
    sleep 1
}
unload_rf() {
    cmd="/sbin/rmmod sf16a18_rf"
    eval $cmd
    sleep 1
}

insmod_mac80211(){
    modprobe mac80211
}

insmod_cfg80211(){
    modprobe cfg80211
}

insmod_umac(){
    all_modparams=
    hb_modparams="
    uapsd_timeout_hb=${uapsd_timeout-0}
    ap_uapsd_on_hb=${ap_uapsd_on-0}
    amsdu_rx_max_hb=${amsdu_rx_max-1}
    ps_on_hb=${ps_on-1}
    amsdu_maxnb_hb=${amsdu_maxnb-6}
    rts_cts_change_hb=${rts_cts_change-2}
    nss_hb=${nss-2}
    "
    lb_modparams="
    uapsd_timeout_lb=${uapsd_timeout-0}
    ap_uapsd_on_lb=${ap_uapsd_on-0}
    amsdu_rx_max_lb=${amsdu_rx_max-1}
    ps_on_lb=${ps_on-1}
    amsdu_maxnb_lb=${amsdu_maxnb-6}
    rts_cts_change_lb=${rts_cts_change-2}
    nss_lb=${nss-2}
    "

    all_modparams=$hb_modparams$lb_modparams
    cmd="/sbin/insmod $1 $all_modparams"
    eval $cmd
}

insmod_fmac() {
    insmod_umac sf16a18_fmac
}

unload_fmac(){
    cmd="/sbin/rmmod sf16a18_fmac"
    eval $cmd
}
