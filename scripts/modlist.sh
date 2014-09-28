#!/bin/bash
modprobe ip6table_filter
modprobe ip6table_mangle
modprobe ip6_tables
modprobe ip6t_REJECT
modprobe iptable_filter
modprobe iptable_mangle
modprobe iptable_nat
modprobe ip_tables
modprobe ipt_MASQUERADE
modprobe ipt_REJECT
modprobe ipv6  
modprobe nf_conntrack 
modprobe nf_conntrack_ipv4 
modprobe nf_conntrack_ipv6
modprobe nf_defrag_ipv4
modprobe nf_defrag_ipv6 
modprobe nf_nat  
modprobe xt_multiport
modprobe vzrst
modprobe vzcpt
service iptables stop
echo 1 > /proc/sys/net/ipv4/ip_forward
iptables -t nat -A POSTROUTING -o $1 -j MASQUERADE
