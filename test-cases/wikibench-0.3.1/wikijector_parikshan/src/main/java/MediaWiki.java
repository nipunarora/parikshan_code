

import java.io.IOException;
import java.net.ConnectException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;
import java.util.Calendar;

import org.apache.http.HttpEntity;
import org.apache.http.HttpException;
import org.apache.http.HttpHost;
import org.apache.http.HttpResponse;
import org.apache.http.HttpVersion;
import org.apache.http.NoHttpResponseException;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.DefaultHttpClientConnection;
import org.apache.http.params.BasicHttpParams;
import org.apache.http.message.BasicHttpEntityEnclosingRequest;
import org.apache.http.message.BasicHttpRequest;
import org.apache.http.params.HttpConnectionParams;
import org.apache.http.params.HttpParams;
import org.apache.http.params.HttpProtocolParams;
import org.apache.http.protocol.BasicHttpProcessor;
import org.apache.http.protocol.HttpContext;
import org.apache.http.protocol.BasicHttpContext;
import org.apache.http.protocol.ExecutionContext;
import org.apache.http.protocol.HttpRequestExecutor;
import org.apache.http.protocol.RequestConnControl;
import org.apache.http.protocol.RequestContent;
import org.apache.http.protocol.RequestExpectContinue;
import org.apache.http.protocol.RequestTargetHost;
import org.apache.http.protocol.RequestUserAgent;
import org.apache.http.util.EntityUtils;


/**
 * This fetcher class uses HttpCore 4.0 from Apache Commons.  It is
 * a bit low-level compared to using httpclient. The advantage is that we do
 * not need additional libraries for logging and such. The increased
 * flexibility is also a big plus for WikiBench, since we need the
 * low-level access HttpCore offers us (like creating our own sockets).
 * 
 * Most of this was created by using and adjusting sample code from HttpCore,
 * since the documentation was a bit sparse as of writing this.
 * 
 * @author E. van Baaren (erikjan@gmail.com)
 * @author Felipe Ledesma Botero (felipe.ledesma@gmail.com)
 * 
 */
public class MediaWiki {
	HttpParams params;
	BasicHttpProcessor httpproc;
	HttpRequestExecutor httpexecutor;
	DefaultHttpClientConnection conn;
	public int lastResponseCode = 0;
	public int lastResponseSize = 0;
	public String serverHost;
	public int serverPort;
	/* This is the boundary used in multi-part/formdata POSTs */
	public String contentBoundary = "-----------------WikiBenchAaB03x";

	/**
	 * Constructor method used to create a new MediaWiki object
	 * @param serverHost	The host name of the MediaWiki server
	 * @param serverPort	The port number of the MediaWiki server
	 */
	public MediaWiki(String serverHost, int serverPort) {
		this.serverHost = serverHost;
		this.serverPort = serverPort;

		httpproc = new BasicHttpProcessor();
		httpexecutor = new HttpRequestExecutor();
		conn = new DefaultHttpClientConnection();
		// Interceptors handle header lines, often just one single line
		// Required protocol interceptors
		httpproc.addInterceptor(new RequestContent());
		httpproc.addInterceptor(new RequestTargetHost());
		// Recommended protocol interceptors
		httpproc.addInterceptor(new RequestConnControl());
		httpproc.addInterceptor(new RequestUserAgent());
		httpproc.addInterceptor(new RequestExpectContinue());

		/* Setup some basic parameters */
		params = new BasicHttpParams();
		HttpProtocolParams.setVersion(params, HttpVersion.HTTP_1_1);
		HttpProtocolParams.setContentCharset(params, "UTF-8");
		HttpProtocolParams.setUserAgent(params, "WikiBench");
		HttpConnectionParams.setConnectionTimeout(params,2000);
		HttpConnectionParams.setSoTimeout(params, 2000);
		HttpConnectionParams.setLinger(params, 0);
		
		/* Do not use Expect: 100-Continue header, since it will complicate
		 * posting data  */
		HttpProtocolParams.setUseExpectContinue(params, false);
	}

	public static void log(String s){
		System.out.println(s);
	}

	/**
	 * Perform a GET operation.
	 * 
	 * @param	serverPath	the full path on the server, including the initial
	 * 						'/' and also including all GET parameters.
	 * @return true if the get request was successful, false otherwise
	 */
	public boolean doGET(String serverPath,int timeout_input ) {
		log("initializing GET request");
		HttpContext context = new BasicHttpContext(null);
		HttpHost host = new HttpHost(serverHost, serverPort);
		//ConnectionReuseStrategy connStrategy = new DefaultConnectionReuseStrategy();
		context.setAttribute(ExecutionContext.HTTP_CONNECTION, conn);
		context.setAttribute(ExecutionContext.HTTP_TARGET_HOST, host);
		this.lastResponseCode = 0;
		this.lastResponseSize = 0;
		
        
		try {
			log("creating socket");
			Socket socket = new Socket();
			log("finished creating socket... now binding with a null");
			socket.bind(null);
			log("finished binding with a null, now setting some of its variables");				
			socket.setSoTimeout(timeout_input);
			socket.setSoLinger(true, 0);
			log("connecting to host...");	
			socket.connect(new InetSocketAddress(host.getHostName(), host.getPort()), timeout_input);
			log("finished connecting, now binding socket with the connection");
			conn.bind(socket, params);		
			conn.setSocketTimeout(timeout_input);
			log("basic http request");
			BasicHttpRequest request = new BasicHttpRequest("GET", serverPath);
			context.setAttribute(ExecutionContext.HTTP_REQUEST, request);
			log("request params");
			request.setParams(params);
			httpexecutor.preProcess(request, httpproc, context);
			log("executing GET");
			HttpResponse response = httpexecutor.execute(request, conn, context);
			response.setParams(params);
			httpexecutor.postProcess(response, httpproc, context);

			/* Get the content, get the size, throw away content */
			String content = EntityUtils.toString(response.getEntity());
			this.lastResponseSize = content.length();
			this.lastResponseCode = response.getStatusLine().getStatusCode();
			log("Last Response Size: " + this.lastResponseSize);
			log("Last Response Code: " + this.lastResponseCode);
			log("shutting down connection");
			conn.shutdown();
			socket.close();

		} catch (UnknownHostException e) {
			e.printStackTrace();
			return false;
		} catch (SocketTimeoutException e) {
			log("Socket timeout!");
			return false;
		} catch (NoHttpResponseException e) {
			log("No http response!");
			return false;
		} catch (ConnectException e) {
			log("Could not connect to remote host. ");
			return false;
		} catch (IOException e) {
			e.printStackTrace();
			return false;
		} catch (HttpException e) {
			e.printStackTrace();
			return false;
		} catch (Exception e) {
			//System.err.println(e.getLocalizedMessage());
			return false;
		} finally {
			try {
				conn.shutdown();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		
		return true;
	}

	
	String constructPost(String wpTextbox1, String wpEdittime) {
		String content=
			multiPartContent("wpSummary", "summary") +
			multiPartContent("wpSection", "") +
			multiPartContent("wpTextbox1", wpTextbox1) +
			multiPartContent("wpEditToken", "+\\") +
			multiPartContent("wpStarttime", getStartTime()) +
			multiPartContent("wpEdittime", wpEdittime)+
			multiPartContent("wpScrolltop", "") +
			multiPartContent("wpAutoSummary", "d41d8cd98f00b204e9800998ecf8427e") +
			multiPartContent("wpSave", "Save page") +
			multiPartTail();
		return content;
	}
	
	/** Format an additional  (field,data) tuple to add to the POST request
	 * 
	 * @return a String containing the field and the data, properly formatted
	 * so it can be appended to a new or existing string with other (field,data)
	 * tuples.
	 */
	private String multiPartContent(String field, String data) {
		return 	"--"+contentBoundary+"\r\n" +
				"Content-Disposition: form-data; name=\""+
				field+"\"\r\n\r\n"
				+data+"\r\n";
	}
	
	/** Format the tail of a multipart/formdata POST
	 * 
	 * @return	a string containing the content boundary and an additional
	 * carriage return to end multipart/formdata request
	 */
	String multiPartTail() {
		return contentBoundary+"\r\n";
	}
	/**
	 * Construct a valid wpEdittime to be used by MediaWiki. This method will 
	 * use the current system time to construct this value
	 * @return a String containing a valid wpEdittime based on current system time
	 */
	private String getStartTime() {
		Calendar now = Calendar.getInstance();
		String year = Integer.toString(now.get(Calendar.YEAR));
		String month = Integer.toString(now.get(Calendar.MONTH));
		String day = Integer.toString(Calendar.DAY_OF_MONTH);
		String hour = Integer.toString(Calendar.HOUR_OF_DAY);
		String minutes = Integer.toString(Calendar.MINUTE);
		String seconds = Integer.toString(Calendar.SECOND);
		return year + month + day + hour + minutes + seconds;
	}

	public static void main(String[] args){
		MediaWiki mywiki = new MediaWiki("138.15.170.140",80);
		if(mywiki.doGET("http://138.15.170.140/index.php/Germany", 200000)){
			System.out.println(" got requests");
		}

	}

}