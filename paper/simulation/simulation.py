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
import decint
#from __future__ import division

def calculate(buf, lam, mu):

    try:

        rho = lam/float(mu)
        p = -1*(mu - lam)*buf
        po = DecInt(exp(DecInt(p)))
    except OverflowError:
        print "Overflow Error Buffer " + str(buf) + " Lambda " + str(lam) + " Mu " + str(mu) + " rho " + str(rho) + " exp power " + str(p)
        return 
    except:
        print "Other Exception"
        return
    
    num1 = (1 - (rho*po))
    den1 = (lam*pow(1-rho,2)*po)

    g1 = num1/den1
    g2 = (buf + (1/mu))/(1-rho)
        
    g  = g1 - g2
    print g
    #print "Buffer " + str(buf) + " Lambda " + str(lam) + " Mu " + str(mu) + " rho " + str(rho) + " exp " + str(p) + " num1 " + str(num1) + " den1 " + str(den1)

    #except:
     #   print "Other error-> Buffer " + str(buf) + " Lambda " + str(lam) + " Mu " + str(mu) + " rho " + str(rho) + " exp power " + str(p) + " po " + str(po) + " num1 " + str(num1) +  

    

def main(argv):

    for x in range(0,40):
        
        #buffer size
        buf = randint(100,200)

        # lambda
        lam = randint(55,60)

        #overhead
        o = randint(25,30)

        #mu
        mu = lam - o

        g = calculate(buf, lam, mu)

    

if __name__ == "__main__":
    main(sys.argv[1:])
