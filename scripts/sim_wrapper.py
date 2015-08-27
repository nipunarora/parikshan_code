# 
# 
# Filename: sim_wrapper.py
# Author: Nipun Arora
# Created: Mon Aug 24 12:16:33 2015 (-0400)
# URL: http://www.nipunarora.net 
# 
# Description: 
# 

import sys
import getopt
import ConfigParser
import subprocess
import os
from io import StringIO


def exec_simulation(sim, lam, mu, buf):

    sim = str(sim)
    lam = str(lam)
    mu = str(mu)
    buf = str(buf)
    cmdline = './queue ' + sim + ' ' + lam + ' ' + mu + ' ' + buf
    #print cmdline
    cmd = subprocess.Popen(cmdline,shell=True)
    cmd.wait()

if __name__ == "__main__":

    sim = 1000000
    buf = 64000    
    lam = 1
    mu = 0.2
    
    for x in range(0,75):
        exec_simulation(sim, lam, mu, buf)
        #buf = buf + 4000
        mu = mu + 0.02
    
