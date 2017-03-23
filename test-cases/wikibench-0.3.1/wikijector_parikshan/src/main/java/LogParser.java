import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;

/**
 * Created by Nipun on 3/22/17.
 */
public class LogParser {

    public Long lowestTime;
    public Long startTime;
    public Long currentTime;
    public String currentPath;

    public static void log(String s){

    }

    private Long parseEpochTime(String s){

        String[] arr;
        arr = s.split("\\.");
        String x;
        if(arr.length>1)
            x = arr[0] + arr[1];
        else
            x = arr[0];

        return Long.parseLong(x);
    }

    public boolean lineParser(String line) {
        System.out.println("Parsing line " + line);
        String[] parts;

        parts = line.split("\\s");				/* Split the trace line on white space */
        //System.out.println(parts);
        currentTime = parseEpochTime(parts[0]);
        String[] x = parts[1].split("wiki");

        currentPath = "http://138.15.170.140/index.php" + x[x.length - 1];

        String currentPostData = parts[2];				/* Get the post data, if any */

        if (currentPostData.length()>4) {		/* Check if there is post data */
			/* In this case we extract the page data and the timestamp */
            int splitPoint = currentPostData.indexOf('|');

            try {
                currentPostData = URLDecoder.decode(currentPostData.substring(splitPoint), "UTF-8");
            } catch (UnsupportedEncodingException e) {
				/* This error should be very rare */
                System.err.println(e.getLocalizedMessage());
                System.err.println("Since there is no UTF-8 support, we will be" +
                        "posting url encoded data. This is ugly!");
            }
        } else if (currentPostData.equals("save")){
			/* Reset the timestamp from any previous parse actions */
            String currentEdittime = null;
			/* Add some dummy data */
            currentPostData = "This content was posted from WikiBench, the" +
                    " benchmarking tool. This content is only posted when there" +
                    "is no content available in the trace file used by WikiBench." +
                    "This can for example happen when the page did not exist yet.";
        }
        log("parsing line (end)");
        return true;
    }
}
