#!/usr/bin/env python

import socket
import SimpleHTTPServer
import SocketServer
import sys, thread, time  

def main(config, errorlog):
    sys.stderr = file(errorlog, 'a')

    for settings in parse(config):
        thread.start_new_thread(server, settings)

    while True:
        time.sleep(60)

def parse(configline):
    settings = list()
    for line in file(configline):
        parts = line.split()
        settings.append((int(parts[0]), int(parts[1]), parts[2], int(parts[3])))
    return settings

def server(*settings):
    try:
        dock_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        dock_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        dock_socket.bind(('', settings[0]))
        dock_socket.listen(5)

        while True:
            client_socket = dock_socket.accept()[0]
            client_data = client_socket.recv(1024)
            sys.stderr.write("[OK] Data received:\n %s \n" % client_data)

            print "Forward data to local port: %s" % (settings[1])
            local_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            local_socket.connect(('', settings[1]))
            local_socket.sendall(client_data)

            print "Get response from local socket"
            client_response = local_socket.recv(1024)
            local_socket.close()

            print "Send response to client"
            client_socket.sendall(client_response)
            print "Close client socket"
            client_socket.close()

            print "Forward data to remote server: %s:%s" % (settings[2],settings[3])
            remote_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            remote_socket.connect((settings[2], settings[3]))
            remote_socket.sendall(client_data)       

            print "Close remote sockets"
            remote_socket.close()
    except:
        print "[ERROR]: ",
        print sys.exc_info()
        raise

if __name__ == '__main__':
    main('multiforwarder.config', 'error.log')
