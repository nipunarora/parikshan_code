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
    buf = 32000    
    lam = 0.5
    mu = 0.1
    
    for x in range(0,200):
        exec_simulation(sim, lam, mu, buf)
        #buf = buf +200
        mu = mu + 0.02
    
