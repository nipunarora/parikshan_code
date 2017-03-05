package org.cs.columbia.nipun.javaTCPProxyRaw;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.SocketTimeoutException;

/**
 * @author nipun
 */
public class CommunicateReplicaTo implements Runnable {

    private boolean isRunning = true;
    private Register register;
    private InputStream in;

    /**
     * Create the SocketListener
     */
    public CommunicateReplicaTo(Register register, InputStream in) {
        this.register = register;
        this.in = in;
    }

    /**
     * Called as a different thread.
     */
    public void run() {
        try {
            //System.out.println("Executing Run");
            String line = null;

            final byte[] request = new byte[1024];
            int bytesRead;

            while (isRunning && (bytesRead = in.read(request)) != -1) {
                /*
                out.write(request, 0, bytesRead);
                out.flush();
                */
                // It may seem obscure to surround it by an if loop, but otherwise we are forced to create a new String object; the parsing to UTF-8 causes some speed issues.
                /*
                if(clientToServer.getProxy().getDebug()) {
                    clientToServer.getProxy().debug("S -> C: " + new String(request, "UTF-8"));
                }
                */

            }

            // The server socket ended!
            register.kill();

        } catch (IOException ex) {
            if(ex instanceof SocketTimeoutException) {
                // The socket simply timed out. Kill, then exit.
                register.kill();
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
