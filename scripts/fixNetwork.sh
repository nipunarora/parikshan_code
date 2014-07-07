# 
# Filename: fixNetwork.sh
# Author: Nipun Arora
# Created: Sun Jul  6 20:49:05 2014 (-0400)
# URL: http://www.nipunarora.net 
# 
# Description: Network access to container
#

#!/bin/bash

iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE 
