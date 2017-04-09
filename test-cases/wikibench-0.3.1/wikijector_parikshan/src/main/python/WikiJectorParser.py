import requests


class WikiJectorParser:


    def readTraceFile(this,tracefile,debug=False):

        file = open(tracefile,'r')

        for line in file:
            print line




if __name__ == "__main__":


print requests.get("http://www.google.com").elapsed.total_seconds()