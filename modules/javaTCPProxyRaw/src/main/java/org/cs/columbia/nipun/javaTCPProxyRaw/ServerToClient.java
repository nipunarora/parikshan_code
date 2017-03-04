package org.cs.columbia.nipun.javaTCPProxyRaw;

import java.io.IOException;
import java.net.SocketTimeoutException;

/**
 * @author nipun
 */
public class ServerToClient implements Runnable {

    private ClientToServer clientToServer;
    private boolean isRunning = true;

    /**
     * Create the SocketListener
     *
     * @param clientToServer The Registry client.
     */
    public ServerToClient(ClientToServer clientToServer) {
        this.clientToServer = clientToServer;
    }

    /**
     * Called as a different thread.
     */
    public void run() {
        try {
            System.out.println("Executing Run");
            String line = null;

            final byte[] request = new byte[1024];
            int bytesRead;

            while (isRunning && (bytesRead = clientToServer.serverIn.read(request)) != -1) {
                clientToServer.clientOut.write(request, 0, bytesRead);
                clientToServer.clientOut.flush();

                // It may seem obscure to surround it by an if loop, but otherwise we are forced to create a new String object; the parsing to UTF-8 causes some speed issues.
                if(clientToServer.getProxy().getDebug()) {
                    clientToServer.getProxy().debug("S -> C: " + new String(request, "UTF-8"));
                }

            }

            // The server socket ended!
            clientToServer.kill();

        } catch (IOException ex) {
            if(ex instanceof SocketTimeoutException) {
                // The socket simply timed out. Kill, then exit.
                clientToServer.kill();
                return;
            }

            //ex.printStackTrace();
            this.kill();
        }
    }

    /**
     * Stops the processing.
     */
    public void kill() {
        isRunning = false;
    }

}
