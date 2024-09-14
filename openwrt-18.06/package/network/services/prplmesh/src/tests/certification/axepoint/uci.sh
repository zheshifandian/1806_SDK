#!/bin/sh

# some wireless configuration needed especialy for axepoint devices

uci batch << 'EOF'
# TODO: The current channel selection does not work correctly when 80Mhz bandwidths are involved.
# This temporary workaround forces the use of 20Mhz bands, and will need to be reverted when the 
# issue is fixed (see https://jira.prplfoundation.org/browse/PPM-258)
set wireless.radio0.htmode='HT20'
set wireless.radio2.htmode='HT20'

################ needed for R2 certification #################
# Enable action/managment frames hostapd notifiecations
set wireless.radio0.notify_action_frame='1'
set wireless.radio2.notify_action_frame='1'

# set protected managment frames capability (pmf) to optional for wireless interfaces (supplicants)
set wireless.default_radio26.pmf='1'
set wireless.default_radio58.pmf='1'

##############################################################

# radios are disabled by default in prplwrt
set wireless.radio0.disabled=0
set wireless.radio2.disabled=0


EOF

uci commit
/etc/init.d/network restart


# System log is currently saved at the end of each test using logread
# which has circular buffer. As a result data could get lost.
# Save system (hostap/driver) logs to file and increace buffer size

uci batch << 'EOF'
set system.@system[0].log_file='/var/log/syslog.txt'
set system.@system[0].log_buffer_size='4096'
set system.@system[0].log_size='4096'
set system.@system[0].log_remote='0'

EOF

uci commit
/etc/init.d/log restart
/etc/init.d/system restart
