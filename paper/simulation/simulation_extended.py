# 
# 
# Filename: simulation.py
# Author: Nipun Arora
# Created: Sun Aug 16 17:31:22 2015 (-0400)
# URL: http://www.nipunarora.net 
# 
# Description: 
# 
import sys
from math import exp,pow
from random import randint
import json
#from __future__ import division

def calculate(buf, lam, mu):

        try:

                rho = lam/float(mu)
                p = -1*(mu - lam)*buf
                po = DecInt(exp(DecInt(p)))

                num1 = (1 - (rho*po))
                den1 = (lam*pow(1-rho,2)*po)

                g1 = num1/den1
                g2 = (buf + (1/mu))/(1-rho)
                g  = g1 - g2

                print "no exception"
                
        except:
                rho = lam/float(mu)
                g = (buf + (1/mu))/(rho - 1) - rho/(lam*(1-rho)*(1-rho))

        data = {}
        data['Buffer Size'] = buf
        data['Lambda'] = lam
        data['Mu'] = mu
        data['Rho'] = rho
        data['Hitting Time'] = g

        print json.dumps(data)        
        #print str(buf) + "," + str(lam) + "," + str(mu) + "," + str(rho) + "," + str(g)

    
def main(argv):

        #lam = float(20)
        lam = float(0.1)
        buf = float(5)
        o = 8

        #for x in range(0,80):
        while lam < 1000: 
                #buffer size
                #buf = randint(2000,8000)
                
                # lambda
                #lam = randint(20,60)

                #overhead
                #o = randint(11,40)/float(10)
                
                #mu
                mu = lam/float(o)

                g = calculate(buf, lam, mu)
                lam = lam + 50

        

if __name__ == "__main__":
    main(sys.argv[1:])
