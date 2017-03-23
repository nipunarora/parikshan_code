import java.util.Scanner;
import static java.lang.Thread.sleep;

import org.kohsuke.args4j.CmdLineException;
import org.kohsuke.args4j.CmdLineParser;
import org.kohsuke.args4j.Option;

/**
 * Created by Nipun on 3/22/17.
 */
public class SimpleWorker {

    @Option(name="-h",usage="Sets a hostname")
    public static String hostname;

    public static void main(String[] args){


        SimpleWorker worker = new SimpleWorker();
        CmdLineParser parser = new CmdLineParser(worker);


        try {
            // parse the arguments.
            parser.parseArgument(args);

        } catch( CmdLineException e ) {
            System.out.println(e.getMessage());
            return;
        }

        System.out.println("Looking at host: " + hostname);

        Scanner sc = new Scanner(System.in);

        boolean flag_first = false;
        Long last_parsed_time;
        Long last_issue_time;

        String serverHost = "localhost";
        int serverPort = 80;
        int timeout = 2000;

        MediaWiki mediaWiki = new MediaWiki(serverHost,serverPort);
        HttpClientExample client = new HttpClientExample();


        while (sc.hasNextLine()) {
            String s = sc.nextLine();
            System.out.println(s);

            LogParser l = new LogParser();
            l.lineParser(hostname, s);
            System.out.println(l.currentTime);

            long before = System.nanoTime();
            //boolean success = mediaWiki.doGET(l.currentPath,timeout);
            try {
                client.sendGet(l.currentPath);
            } catch (Exception e) {
                e.printStackTrace();
            }
            long after = System.nanoTime();

            //System.out.println(" Report " + success + " Time " + (after-before));

        }
    }
}
