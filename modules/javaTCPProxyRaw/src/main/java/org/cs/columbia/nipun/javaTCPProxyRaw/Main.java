package org.cs.columbia.nipun.javaTCPProxyRaw;

import org.cs.columbia.nipun.javaTCPProxyRaw.Proxy;

/**
 * @author nipun
 */
public class Main {

    public static void main(String[] args) {
        String out = "127.0.0.1";
        String listenIP = "0.0.0.0";
        int port = 1358;
        int listenPort = 1357;
        boolean duplicate = false;
        String replica = "127.0.0.1";
        int replicaPort = 1356;
        boolean debug = false;

        // Parse through arguments.
        for(int i = 0; i < args.length; i++) {
            // Look for out (-o, --out)
            if(args[i].equalsIgnoreCase("-l") || args[i].equalsIgnoreCase("--listen")) {

                String[] split = args[i + 1].split(":");
                listenIP=split[0];
                listenPort = Integer.valueOf(split[1]);
            }
            if(args[i].equalsIgnoreCase("-o") || args[i].equalsIgnoreCase("--output")) {
                String[] split = args[i + 1].split(":");
                out=split[0];
                port = Integer.valueOf(split[1]);
            }
            if(args[i].equalsIgnoreCase("-r") || args[i].equalsIgnoreCase("--replica")) {
                duplicate = true;
                String[] split = args[i + 1].split(":");
                replica=split[0];
                replicaPort = Integer.valueOf(split[1]);
            }
            // Look for debug (-d, --debug)
            if(args[i].equalsIgnoreCase("-d") || args[i].equalsIgnoreCase("--debug")) {
                debug = true;
                System.out.println("Debug mode activated.");
            }
        }

        // Initiate the Proxy
        Proxy proxy = new Proxy();
        proxy.setHost(out);
        proxy.setPort(port);
        proxy.setListeningIP(listenIP);
        proxy.setListeningPort(listenPort);
        proxy.setDebug(debug);
        proxy.setReplicaIP(replica);
        proxy.setReplicaPort(replicaPort);
        proxy.setDuplicate(duplicate);

        // Start the proxy.
        proxy.start();
    }

}
