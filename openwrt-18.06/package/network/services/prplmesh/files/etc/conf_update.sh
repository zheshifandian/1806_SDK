#!/bin/sh

rm -rf /tmp/agent_is_ok

for i in $(seq 60); do
    sleep 2
    if cat /tmp/agent_is_ok > /dev/NULL; then
		logger -t easymesh agent is ok
        break
    else
        continue
    fi
done

restart()
{
    /bin/expect <<-EOF
    spawn ssh root@$1
    expect {
    "continue connecting" {exp_send "yes\r"; exp_continue}
    "password:" {exp_send "admin\r"}
    }
    expect "#"
    send "/etc/init.d/prplmesh restart \r"
    expect "#"

    expect "#"
    send "exit\r"
    expect eof
EOF
}

logger -t easymesh start upgrade
arp-scan -l -g | grep '10:16:88' | awk -F ' ' '{print$1}' | while read line; do restart $line ; done
logger -t easymesh end upgrade
