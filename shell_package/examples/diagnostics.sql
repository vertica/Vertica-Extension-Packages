-- script to collect diagnostics through vsql instead of ssh + admintools
create local temp table diagnostics (cmd varchar(1000)) on commit preserve rows unsegmented all nodes;

copy diagnostics from stdin direct delimiter '&';
ulimit -a
uname -a
rpm -q vertica
if [ -f '/etc/redhat-release' ]; then /bin/cat /etc/redhat-release; fi
if [ -f '/etc/SuSE-release' ]; then /bin/cat /etc/SuSE-release; fi
/bin/hostname
uptime
/bin/ping -c 1 `hostname`
/bin/ping -c 1 $HOSTNAME
/bin/cat /etc/hosts
/sbin/ifconfig
/bin/netstat -tuan
/bin/netstat -s
/bin/cat /etc/resolv.conf
/bin/cat /etc/sysctl.conf
/bin/cat /etc/security/limits.conf
/bin/cat /proc/cpuinfo
/bin/cat /proc/vmstat
/bin/cat /proc/meminfo
/bin/cat /proc/sys/vm/swappiness
df -H
if [ -e /usr/bin/lsof ]; then /usr/bin/lsof | grep vertica | wc -l ; else /usr/sbin/lsof | grep vertica | wc -l; fi
if [ -f '/usr/bin/sar' ]; then /usr/bin/sar -r; fi
free
date
echo $LANG
echo $LC_ALL
echo $LC_CTYPE
ls -l / | grep tmp
ls -la %s
if [ -e /proc/net/bonding ]; then cat /proc/net/bonding/bond*; else echo 'Bonding not found'; fi
if [ -e /etc/modprobe.conf ]; then cat /etc/modprobe.conf; else echo '/etc/modprobe.conf not found'; fi
if [ -d /etc/sysconfig/network-scripts ]; then for nic in `ls /etc/sysconfig/network-scripts/ifcfg*`;do echo 
if [ -d /etc/sysconfig/network/scripts ]; then for nic in `ls /etc/sysconfig/network/ifcfg-*`;do echo 
if [ -e /etc/network/interfaces ]; then cat /etc/network/interfaces; fi
for dev in `ls /sys/block`;do  echo 
ps -ef 
dmesg | tail --lines=+1000
ps ax -o vsz,rss,pid,args | sort -k1 -g -r 
\.

select shell_execute(node_name,cmd) over (partition by segval) from onallnodes, diagnostics order by id;

drop table diagnostics;