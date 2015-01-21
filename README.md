==================
SANDBOX TESTING
=================

:version: 1.1
:source: https://github.com/nipunarora/sandbox-testing/wiki/Parikshan:-A-Testing-Harness-for-In-Vivo-Sandbox-Testing
:keywords: how-to, sandbox testing

Decription
--------

One of the biggest problems faced by developers testing large scale systems is replicating the deployed environment to figure out errors.
In recent years there has been a lot of work in record-and-replay systems which captures traces from live production systems, and replays them.
However, most such record-replay systems have a high recording overhead and are still not practical to be used in production environments without paying a penalty in terms of overhead.

In this work we present a testing harness for production systems which allows the capabilities of running test-cases in a sandbox environment in the wild at any point in the execution of an integrated application. 
The paper levarages, User-Space Containers(OpenVZ/LXCs) to launch test instances in a container cloned and migrated from a running instances of an application. 
The test-container provides a sandbox environment, for safe execution of test-cases provided by the users without disturbing the execution environment. 
Test cases are initiated using user-defined probe points which launch test-cases using the execution context of the probe point. 
Our sandboxes provide a seperate namespace for the processes executing the test cases, replicate and copy inputs to the parent application, safetly discard all outputs, and manage the file system such that existing and newly created file descriptors are safetly managed.

We believe our tool provides a mechanism for practical testing of large scale multi-tier and cloud applications. 

1. Installing OpenVZ
---------------

Install OpenVZ via the following script: https://github.com/nipunarora/sandbox-testing/blob/master/openvz-boot.sh

Install OpenVZ in Debian Wheezy: http://www.howtoforge.com/installing-and-using-openvz-on-debian-wheezy-amd64

Install Ubuntu 13.04 instructions: http://www.howtoforge.com/installing-and-using-openvz-on-ubuntu-13.04-amd64

Install OpenVZ in Centos 6.4: http://www.howtoforge.com/installing-and-using-openvz-on-centos-6.4

Install vzdump http://chrisschuld.com/2009/11/installing-vzdump-for-openvz-on-centos/

Installing OpenVZPanel http://owp.softunity.com.ru

1. Using vzmigrate
---------------
 vzmigrate --online <host> VEID

2. Using vzdump
--------------
http://www.howtoforge.com/clone-back-up-restore-openvz-vms-with-vzdump

3. CRIU install
--------------
http://xmodulo.com/2013/05/how-to-checkpoint-and-restore-linux-process.html

4. Everything about vzctl
-----------------
http://openvz.org/Man/vzctl.8

5. Ploop Articles
-----------
http://openvz.livejournal.com/40830.html http://openvz.org/Ploop/Backup

6. Proxmox
-----------
http://www.proxmox.com/downloads http://pve.proxmox.com/wiki/Installation#Proxmox_VE_web_interface

7. Backup and Restore
------------
https://github.com/andreasfaerber/vzpbackup Blog Example: http://blog.maeh.org/blog/2013/09/03/openvz-ploop-backup-and-restore-scripts/

8. To fix any networking issue
--------------
Venet Venet routed networking is probably the simplest to set up, simply add the IP address to the VE:

[host-node]# vzctl set 101 --ipadd 192.168.2.1 --save

After this the host should be able to ping the VE. To allow the VE to access the rest of the LAN we must enable forwarding and masquerading, as all activity on the LAN must look like it is coming directly from host (with its IP address).

First stop and flush default iptables

service iptables stop

Set ip_forwarding for ipv4

[host-node]# echo 1 > /proc/sys/net/ipv4/ip_forward

Set iptables to MASQUERADE on eth0
[host-node]# iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE

replace /etc/modprobe.d/openvz.conf from 1 to 0

To allow for cloning to work - 
http://wiki.hillockhosting.com/openvz/vzrst-module-is-not-loaded-on-the-destination-node/

modprobe vzrst

modprobe vzcpt

9. Splitting Network Traffic
----------------
Need to split traffic at Layer 7 level

IPTables V Tunnel: http://serverfault.com/questions/570761/how-to-duplicate-tcp-traffic-to-one-or-multiple-remote-servers-for-benchmarking

Agnoster Duplicator: https://github.com/agnoster/duplicator

10. Starting a ploop container
---------------

I want to use CentOS 6 in my virtual machines, so I download a CentOS 6 template:

cd /vz/template/cache

wget http://download.openvz.org/template/precreated/centos-6-x86_64.tar.gz

I will now show you the basic commands for using OpenVZ.

To set up a VPS from the CentOS 6 template, run:

vzctl create 101 --ostemplate centos-6-x86_64 --layout ploop --config basic

The 101 must be a uniqe ID - each virtual machine must have its own unique ID. You can use the last part of the virtual machine's IP address for it. For example, if the virtual machine's IP address is 192.168.0.101, you use 101 as the ID.

To set a hostname and IP address for the vm, run:

vzctl set 101 --hostname test.example.com --save

vzctl set 101 --ipadd 192.168.0.101 --save

Next we set the number of sockets to 120 and assign a few nameservers to the vm:

vzctl set 101 --numothersock 120 --save

vzctl set 101 --nameserver 8.8.8.8 --nameserver 8.8.4.4 --nameserver 145.253.2.75 --save


11. Network Card Management in KVM
-------------

The assignment of network cards to interfaces is done in /etc/udev/rules.d/70-persistent-net.rules you need to flush the lines, if you add a new NIC

12. Install Node JS & npm
-------------

Ref: https://github.com/joyent/node/wiki/backports.debian.org, https://github.com/joyent/node/wiki/Installing-Node.js-via-package-manager

Run the following (as root):

echo "deb http://ftp.us.debian.org/debian wheezy-backports main" >> /etc/apt/sources.list
apt-get update
apt-get install nodejs-legacy
curl --insecure https://www.npmjs.org/install.sh | bash


13. Proxy Servers
----------
http://voorloopnul.com/blog/a-python-proxy-in-less-than-100-lines-of-code/ - simple proxy 
https://github.com/iSECPartners/tcpprox - well made proxy 
https://gist.github.com/fiorix/1878983 - Twisted proxy

Vaurien Install 

- yum install python python-devel
- yum install libevent-devel
- pip install vaurien

 

14. Evaluation Install
------------------

LAMP Install - https://www.digitalocean.com/community/tutorials/how-to-install-linux-apache-mysql-php-lamp-stack-on-ubuntu-14-04
Httperf - https://cs.uwaterloo.ca/~brecht/servers/openfiles.html - fixing file descriptors


15. Todo
------------

 vzmigrate --times --online <host> VEID 


16. Small Networking Issues
------------

Change ssh access to not use DNS and GGSAPI::

       useDNS no
       GGSAPIAuthentication no

Start mysqld server using ./bin/mysqld_safe --skip-name-resolve --user=mysql