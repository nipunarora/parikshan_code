package org.cs.columbia.nipun.javaTCPProxyRaw;

import java.io.*;
import java.net.ConnectException;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.nio.channels.Pipe;

/**
 * @author nipun
 */
public class Register{

    private Pipe pipe;

    public PipedInputStream pis;
    public PipedOutputStream pos;

    private Proxy proxy;
    private boolean isRunning = true;

    private Socket outsocket;
    private Socket insocket;
    private Socket replicaSocket;

    public InputStream serverIn;
    public OutputStream serverOut;

    public InputStream clientIn;
    public OutputStream clientOut;

    public InputStream replicaIn;
    public OutputStream replicaOut;

    private Communicate ClienttoServer;
    private Communicate ServertoClient;

    //Incase of duplicate
    private CommunicateToServerAndPipe ClientToServerAndPipe;
    private CommunicatePipeToSock PipeToSock;
    private CommunicateReplicaTo CommunicateFromReplica;

    public String client;
    public String server;

    /**
     * Creates a Registry instance; Registry represents a client.
     *
     * @param proxy The Proxy instance.
     * @param inSocket The socket to the client.
     * @throws IOException Upon getting an exception, the program will throw an exception.
     */
    public Register(Proxy proxy, Socket inSocket){
        this.proxy = proxy;

        try {
            this.outsocket = new Socket(proxy.getHost(), proxy.getPort());
            this.insocket = inSocket;

            // Initiate the in and out fields.
            this.clientIn = this.insocket.getInputStream();
            this.clientOut = this.insocket.getOutputStream();

            this.serverIn = this.outsocket.getInputStream();
            this.serverOut = this.outsocket.getOutputStream();

            pipe = Pipe.open();


        } catch (ConnectException ex) {
            proxy.debug("Received connection error while creating Socket to outsource.");
            // do nothing, kill process.
            this.kill();
            return;
        } catch (IOException ex) {
            // do nothing, kill process.
            this.kill();
            return;
        }

        // Start up the SocketListener.
        this.ClienttoServer = new Communicate(this,this.clientIn,this.serverOut);
        Thread thread1 = new Thread(this.ClienttoServer);
        thread1.start();

        this.ServertoClient = new Communicate(this, this.serverIn,this.clientOut);
        Thread thread2 = new Thread(this.ServertoClient);
        thread2.start();

    }

    /**
     * Creates a Registry instance; Registry represents a client.
     *
     * @param proxy The Proxy instance.
     * @param inSocket The socket to the client.
     * @throws IOException Upon getting an exception, the program will throw an exception.
     */
    public Register(Proxy proxy, Socket inSocket, boolean flag){
        this.proxy = proxy;

        try {
            this.outsocket = new Socket(proxy.getHost(), proxy.getPort());
            this.replicaSocket = new Socket(proxy.getReplicaIP(),proxy.getReplicaPort());
            this.insocket = inSocket;

            // Initiate the in and out fields.
            this.clientIn = this.insocket.getInputStream();
            this.clientOut = this.insocket.getOutputStream();

            this.serverIn = this.outsocket.getInputStream();
            this.serverOut = this.outsocket.getOutputStream();

            this.replicaIn = this.replicaSocket.getInputStream();
            this.replicaOut = this.replicaSocket.getOutputStream();


            pis = new PipedInputStream(100000000);
            pos = new PipedOutputStream();
            pos.connect(pis);
            //pipe = Pipe.open();


        } catch (ConnectException ex) {
            proxy.debug("Received connection error while creating Socket to outsource.");
            // do nothing, kill process.
            this.kill();
            return;
        } catch (IOException ex) {
            // do nothing, kill process.
            this.kill();
            return;
        }

        // Start up the SocketListener.
        this.ClientToServerAndPipe = new CommunicateToServerAndPipe(this,this.clientIn,this.serverOut);
        Thread thread1 = new Thread(this.ClientToServerAndPipe);
        thread1.start();

        this.ServertoClient = new Communicate(this, this.serverIn,this.clientOut);
        Thread thread2 = new Thread(this.ServertoClient);
        thread2.start();

        this.PipeToSock = new CommunicatePipeToSock(this,this.replicaOut);
        Thread thread3 = new Thread(this.PipeToSock);
        thread3.start();

        this.CommunicateFromReplica = new CommunicateReplicaTo(this,this.replicaIn);
        Thread thread4 = new Thread(this.CommunicateFromReplica);
        thread4.start();
    }

    /**
     * Ran as a separate thread.
     */
    /*
    public void run() {
        try {
            String line = null;

            final byte[] request = new byte[1024];

            int bytesRead;

            // Try and read lines from the client.
            while (isRunning && (bytesRead = this.clientIn.read(request)) != -1) {
                this.serverOut.write(request, 0, bytesRead);
                this.serverOut.flush();

                // It may seem obscure to surround it by an if loop, but otherwise we are forced to create a new String object; the parsing to UTF-8 causes some speed issues.
                if(this.getProxy().getDebug()) {
                    this.getProxy().debug("C -> S: " + new String(request, "UTF-8"));
                }
            }

            // Client disconnected.
            this.kill();

        } catch (IOException ex) {
            if(ex instanceof SocketTimeoutException) {
                // The socket simply timed out. Kill, then exit.
                this.kill();
                return;
            }

            //ex.printStackTrace();
            this.kill();
        }
    }
    */

    /**
     * Gets the Proxy of the Registry.
     *
     * @return proxy
     */
    public Proxy getProxy() {
        return this.proxy;
    }

    /**
     * Kills the Registry, this happens when either the client or server disconnects.
     */
    public void kill() {
        if(this.ClienttoServer != null) this.ClienttoServer.kill();
        isRunning = false;

        try {
            proxy.debug("Client " + insocket.getInetAddress().getHostAddress() + " has disconnected or connection lost");

            if(this.outsocket != null) this.outsocket.close();
            if(this.insocket != null) this.insocket.close();
            if(this.replicaSocket !=null) this.replicaSocket.close();

        } catch (IOException ex) {
            // Do nothing.
        }
    }

}