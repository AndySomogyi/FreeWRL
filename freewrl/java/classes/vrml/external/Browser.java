// Specification of the External Interface for a VRML applet browser.
// FreeWRL Viewer Interface - bypass netscape and go directly
// to the viewer. 

package vrml.external;


import java.util.*;
import java.applet.*;
import java.awt.*;
import java.net.*;
import java.io.*;
import java.lang.reflect.*;
import vrml.external.Node;
import vrml.external.field.EventOut;
import vrml.external.field.EventOutObserver;
import vrml.external.field.EventInMFNode;
import vrml.external.FreeWRLEAI.EAIoutThread;
import vrml.external.FreeWRLEAI.EAIinThread;
import vrml.external.FreeWRLEAI.EAIAsyncThread;
import vrml.external.exception.InvalidNodeException;
import vrml.external.exception.InvalidVrmlException;

public class Browser implements BrowserInterface

{
    // The thread that reads and processes FreeWRL EAI replies...
     Thread 		FreeWRLThread;

    // The Thread that sends stuff to the EAI port for FreeWRL...
    static EAIoutThread 		EAIoutSender;


    // The following are used to send to/from the FreeWLR Browser...
    ServerSocket	EAISocket;
    Socket		sock;
    static PrintStream         EAIout;

    // The following pipe listens for replies to events sent to
    // the FreeWRL VRML viewer via the EAI port.

    private PipedInputStream EAIfromFreeWRLStream;
    static DataInputStream EAIfromFreeWRLInputStream;

    private String              reply = "";

    
    // Events. EVno is the "highest +1" registered event number...
    // EVarray corresponds to the events returned by FreeWRL  to our
    // type, EVtype is the type as registered.
    
    public static int	EVno = 0;
    public static int	EVarray [] = new int[256];
    public static int	EVtype [] = new int[256];
    public static boolean   EVshortreply [] = new boolean[256];
    public static Object EVObject[] = new Object[256];
    public static EventOutObserver EVObserver[] = new EventOutObserver[256];
    
    // The FreeWRL browser sends us changes to variables if/when they
    // are updated. We tell the FreeWRL viewer what variables to look at
    // by giving it a register listener command. The EAIinThread thread
    // will send responses to the getVRMLReply procedure (below), or, if
    // it receives an event, will send the result to the RL_Async thread...
    
    static EAIAsyncThread        RL_Async;
    
    // Query Number as sent to the FreeWRL Browser.
    static int   queryno = 1;
    
    
    // Sending to FreeWRL needs to synchronize on an object;
    static Object FreeWRLToken = new Object();
    

    // Interface methods.
    public int get_Browser_EVtype (int event)
      {
	// System.out.println ("get_Browser_EVtype is returning " + EVtype[event]);
        return EVtype[event];
      }

    public EventOutObserver get_Browser_EVObserver (int eventno)
      {
	// System.out.println ("get_Browser_EVObserver is returning " +  EVObserver[eventno]);
        return EVObserver[eventno];
      }

    public boolean get_Browser_EV_short_reply (int event)
      {
	int EVcounter;
        for (EVcounter=0; EVcounter<Browser.EVno; EVcounter++) {
          if (EVarray[EVcounter] == event) {
            break;
          }
        }
	// System.out.println ("get_Browser_EV_short_reply is returning " + EVshortreply[EVcounter]);
        return EVshortreply[EVcounter];

      }

    public void Browser_RL_Async_send (String EVentreply, int eventno) 
      {
        int EVcounter;
        for (EVcounter=0; EVcounter<Browser.EVno; EVcounter++) {
          if (Browser.EVarray[EVcounter] == eventno) {
            break;
          }
        }
	// System.out.println ("Browser_RL_Async_send sending " + EVentreply + " to number " + EVcounter);
        RL_Async.send(EVentreply, EVcounter);
      }

    // Associates this instance with the first embedded plugin in the current frame.
    public Browser(Applet pApplet) {

  	// Create a socket here for an EAI server on localhost
	int incrport = -1;
	EAISocket = null;

	// This was an attempt to make multi-freewrls run on one machine...
	// while ((EAISocket == null) && (incrport < 30))
  	// try {
	//	incrport = incrport + 1;
  	//	EAISocket = new ServerSocket(2000 + incrport);
  	//} catch (IOException e) {
  	//  System.out.print ("Browser: Error creating socket for FreeWRL EAI on port " + incrport + "\n");
  	//}
  
	// Lets just do this with one socket...

	try {
		EAISocket = new ServerSocket(2000);
	} catch (IOException e) {
		System.out.println ("Browser: Error creating socket for FreeWRL EAI on port 2000");
	}
	System.out.println ("Browser: opened port on port 2000 waiting for data" );


  	try {
  		sock=EAISocket.accept();
  	} catch (IOException e) {
  	  System.out.print ("Browser: error creating sub-scoket in FreeWrl Javascript\n");
  	}
  	// Start the readfrom FREEWRL thread...
   	FreeWRLThread = new Thread ( new EAIinThread(sock, pApplet, this));
           FreeWRLThread.start();
  
  	// Open the pipe for EAI replies to be sent to us...
        try {
          EAIfromFreeWRLStream = new PipedInputStream (EAIinThread.EAItoBrowserStream);
          EAIfromFreeWRLInputStream = new DataInputStream (EAIfromFreeWRLStream);	
        } catch (IOException ie) {
          System.out.println (ie);
        }
  
  	// Wait for the FreeWRL browser to send us something...
        try {
          System.out.println (EAIfromFreeWRLInputStream.readLine());
        } catch (IOException ie) {System.out.println (ie);}
  
  	// Send the correct response...
	try {
		EAIout = new PrintStream (sock.getOutputStream());
		EAIout.print ("FreeWRL EAI Serv0.27");
		EAIout.flush ();
	} catch (IOException e) {
		System.out.print ("error on reiniting output stream");
	}
  	// Browser is "gotten", and is started.

  	// Start the SendTo FREEWRL thread...
	EAIoutSender = new EAIoutThread(EAIout);
        EAIoutSender.start();

	// Start the thread that allows Registered Listenered
	// updates to come in.
	RL_Async = new EAIAsyncThread();
	RL_Async.start();

  	return;
    }
  
    // construct an instance of the Browser class
    // If frameName is NULL, current frame is assumed.
    public Browser(Applet pApplet, String frameName, int index) {
      System.out.println ("Browser2 Not Implemented");
  
    }
  
    public String        getVersion() {
      System.out.println ("getVersion Not Implemented");
  
       return "0.17";
     }
  
    // Get the current velocity of the bound viewpoint in meters/sec,
    // if available, or 0.0 if not
    public float         getCurrentSpeed() {
      System.out.println ("getCurrentSpeed Not Implemented");
  
      return (float) 0.0;
    }
  
    // Get the current frame rate of the browser, or 0.0 if not available
    public float         getCurrentFrameRate() {
  
    String retval;

    synchronized (FreeWRLToken) {
      EAIoutSender.send ("" + queryno + "\nGCFR\n");
      retval = getVRMLreply(queryno);
      queryno += 1;
    }

    return Float.valueOf(retval).floatValue();
    }
  
    // Get the URL for the root of the current world, or an empty string
    // if not available
    public String        getWorldURL() {

      String retval;

      
       synchronized (FreeWRLToken) {
         EAIoutSender.send ("" + queryno + "\nGWU\n");
         retval = getVRMLreply(queryno);
         queryno += 1;
       }
      return retval;

    }
  
    // Replace the current world with the passed array of nodes
    public void          replaceWorld(Node[] nodes)
         throws IllegalArgumentException {
         throw new IllegalArgumentException ("replaceWorld Not Implemented");
    } 
  
  
    // Load the given URL with the passed parameters (as described
    // in the Anchor node)
    public void          loadURL(String[] url, String[] parameter) {
      System.out.println ("loadURL Not Implemented");
  
      return;
    }
  
  
    // Set the description of the current world in a browser-specific
    // manner. To clear the description, pass an empty string as argument
    public void          setDescription(String description) {
      System.out.println ("setDescription Not Implemented");
  
      return;
    }
  
  
    // Parse STRING into a VRML scene and return the list of root
    // nodes for the resulting scene
    public Node[]        createVrmlFromString(String vrmlSyntax) 
			throws InvalidVrmlException {

      Node[]  x = {new Node()};
      StringTokenizer tokens;
      String retval;
      String temp;
      int count;

      synchronized (FreeWRLToken) {
        EAIoutSender.send ("" +queryno + "\nCVS "+vrmlSyntax+"\nEOT\n");
        retval = getVRMLreply(queryno);

        tokens = new StringTokenizer (retval);
        count = 0;

        while (tokens.hasMoreTokens()) {
          x[count] = new Node();
          x[count].NodeName = tokens.nextToken();
          count ++;
        }
        queryno += 1;
      }
      return x;
    }
  
  
  
    // Tells the browser to load a VRML scene from the passed URL or
    // URLs. After the scene is loaded, an event is sent to the MFNode
    // eventIn in node NODE named by the EVENT argument
    public void          createVrmlFromURL(String[] url,
                                           Node node,
                                           String event) {

      EventInMFNode		Evin;
      String retval;
      Node[]  x = {new Node()};
      StringTokenizer tokens;
      int count;


       synchronized (FreeWRLToken) {
         EAIoutSender.send ("" + queryno + "\nCVU " + url[0] + "\n");

         retval = getVRMLreply(queryno);

         tokens = new StringTokenizer (retval);
         count = 0;
 	x = new Node[tokens.countTokens()];

         while (tokens.hasMoreTokens()) {
           x[count] = new Node();
           x[count].NodeName = tokens.nextToken();
           count ++;
         }
 	queryno += 1;
       }

      // Now, sent the event to the event!
      Evin = (EventInMFNode) node.getEventIn(event);
      Evin.setValue(x);
    }
  
  
    // Add and delete, respectively, a route between the specified eventOut
    // and eventIn of the given nodes
    public void          addRoute(Node fromNode, String fromEventOut,
                                  Node toNode, String toEventIn) throws
				  IllegalArgumentException {
      throw new IllegalArgumentException ("AddRoute Not Implemented");
    }
  
  
    public void          deleteRoute(Node fromNode, String fromEventOut,
                                     Node toNode, String toEventIn) 
			 throws IllegalArgumentException {
      throw new IllegalArgumentException ("DeleteRoute Not Implemented");
    }
  
    // begin and end an update cycle
    public void          beginUpdate() {} 
    public void          endUpdate() {} 
  
    // called after the scene is loaded, before the first event is processed
    public void initialize() {
      System.out.println ("Initialize Not Implemented");
  
    }
    
  
    // called just before the scene is unloaded
    public void shutdown() {
      System.out.println ("Shutdown Not Implemented");
  
    }
  
    // return an instance of the Browser class
    // This returns the first embedded plugin in the current frame.
    static public Browser getBrowser(Applet pApplet) {
      return (new Browser(pApplet));
    }
  
    // return an instance of the Browser class
    // If frameName is NULL, current frame is assumed.
    static public Browser getBrowser(Applet pApplet, String frameName, int index) {
      // We don't have frames and indexes yet...
      Browser x = getBrowser(pApplet);
      System.out.println ("getBrowser possibly Not Implemented");
  
      return x;
    }
  
  
    // Get a DEFed node by name. Nodes given names in the root scene
    // graph must be made available to this method. DEFed nodes in inlines,
    // as well as DEFed nodes returned from createVrmlFromString/URL, may
    // or may not be made available to this method, depending on the
    // browser's implementation
  
    public Node getNode (String NodeName) throws InvalidNodeException
      {
      Node temp;

      temp = new Node();  

      synchronized (FreeWRLToken) {
        EAIoutSender.send ("" + queryno + "\nGN " + NodeName + "\n");
        temp.NodeName = getVRMLreply (queryno);
        queryno += 1;
      }
      if (temp.NodeName.equals("undefined"))
	throw new InvalidNodeException(NodeName + "undefined");

      return temp;
      }




  //
  // Send Event to the VRML Browser. Note the different methods, depending
  // on the parameters.
  //
  // SendChildEvent waits for confirmation that child is added/removed to MFNode array.
  // This gets around the problem of sending two adds in succession, and having
  // the second overwrite the first.
  public static void SendChildEvent (String NodeName, String FieldName, String Value)
    {
      String retval;

      synchronized (FreeWRLToken) {
        EAIoutSender.send ("" + queryno + "\nSC " + NodeName + " " + 
           FieldName + "\n" + Value + "\n");
        retval = getVRMLreply(queryno);
        queryno += 1;
	
        // Now, tell FreeWRL to update routes
        EAIoutSender.send ("" + queryno + "\nUR " + NodeName + " " + 
           FieldName + "\n" + Value + "\n");
        retval = getVRMLreply(queryno);
        queryno += 1;
      }
      return;
    }

  // Most events don't need us to wait around for it.
  public static void SendEvent (String NodeName, String FieldName, String Value)
    {

      synchronized (FreeWRLToken) {
        EAIoutSender.send ("" + queryno + "\nSE " + NodeName + " " + 
           FieldName + "\n" + Value + "\n");
        // JAS - don't wait. getVRMLreply(queryno);
        queryno += 1;
      }
      return;
    }

  // Get the browser name
  public static String getName() {
      String retval;

       synchronized (FreeWRLToken) {
         EAIoutSender.send ("" + queryno + "\nGNAM\n");
         retval = getVRMLreply(queryno);
         queryno += 1;
       }
      return retval;
  }


  protected static String SendEventType (String NodeName, String FieldName) {
      // get a type from a particular node.

      String retval;

      synchronized (FreeWRLToken) {
        EAIoutSender.send ("" + queryno + "\nGT " + NodeName + " " +
           FieldName + "\n");
        retval = getVRMLreply(queryno);
        queryno += 1;
      }
System.out.println ("SendEventType returning " + retval);
      return retval;
}

  public static String SendEventOut (String NodeName, String FieldName) {
      // get a value from a particular node.

      String retval;

       synchronized (FreeWRLToken) {
         EAIoutSender.send ("" + queryno + "\nGV " + NodeName + " " +
            FieldName + "\n");
         retval = getVRMLreply(queryno);
         queryno += 1;
      }
     return retval;
}

  public static void RegisterListener (EventOutObserver f, Object userData,
			String outNode, String command,  int EventType)
    {
       synchronized (FreeWRLToken) {
         EAIoutSender.send ("" + queryno + "\nRL " + outNode + 
 		" " + command + " " + queryno + "\n");
 
         EVarray [EVno] =  queryno;
         EVtype [EVno] = EventType;     
         EVObject[EVno] = userData;
         EVObserver[EVno] = f;

	 // Is this a short, consise answer type? 
	 // (see field/FieldTypes.java for more info)

	 switch (EventType) {
		case 1:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
		case 17:
		case 18:
		case 19:
			EVshortreply[EVno] = true;
			break;
		default:
			EVshortreply[EVno] = false;
	 } 


         EVno += 1;
       
         getVRMLreply(queryno); 
         queryno += 1;
       }
    }

    protected synchronized static String getVRMLreply (int queryno) 
      {
  
        String req = "0"; 
        String rep = "";
  

 	// This waits for the correct event toe be returned. Note that
	// sendevents dont wait, so there is the possibility that
	// we will have to while away a bit of time waiting...
 
 	while (queryno != Integer.parseInt(req)) { 
           try {
             req = Browser.EAIfromFreeWRLInputStream.readLine();
           } catch (IOException ie) {System.out.println (ie);}

           if (queryno != Integer.parseInt(req)) {
             System.out.println ("getVRMLreply - REPLIES MIXED UP!!! Expecting " 
                + queryno + 
   	     " got " + req);
           }
       
           try {
                 rep = Browser.EAIfromFreeWRLInputStream.readLine(); 
           } catch (IOException ie) { System.out.println ("getVRMLreply failed");}
	

         }

        return rep; 
      }
  
}

