package org.cs.columbia.nipun.javaTCPProxyRaw;

import java.util.HashSet;

/**
 * A defunct class
 * @author nipun
 */
public class RegistryManager {
    private HashSet<ClientToServer> clients = new HashSet<ClientToServer>();
    private Proxy proxy;

    /**
     * Creates the RegistryManager instance.
     *
     * @param proxy
     */
    public RegistryManager(Proxy proxy) {
        this.proxy = proxy;
    }

    /**
     * Adds a Client to the Registry.
     *
     * @param client
     */
    public void addClient(ClientToServer client) {
        clients.add(client);
    }

    /**
     * Removes a Client from the Registry.
     *
     * @param client
     */
    public void removeClient(ClientToServer client) {
        clients.remove(client);
    }

}
