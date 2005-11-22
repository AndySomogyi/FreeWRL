package vrml.external;

import vrml.external.Node;
import vrml.external.exception.InvalidVrmlException;

public interface IBrowser {
	public String getName();
	public String getVersion();
	public float getCurrentSpeed();
	public float getCurrentFrameRate();
	public String getWorldURL();
	public void replaceWorld(Node[] nodes) throws IllegalArgumentException;
	public void loadURL(String[] url, String[] parameter);
	public void setDescription(String description);
	public Node[] createVrmlFromString(String vrmlSyntax) throws InvalidVrmlException;
	public void createVrmlFromURL(String[] url, Node node, String event);
	public Node getNode(String name);
	public void addRoute(Node fromNode, String fromEventOut, Node toNode, String toEventIn) throws IllegalArgumentException;
	public void deleteRoute(Node fromNode, String fromEventOut, Node toNode, String toEventIn) throws IllegalArgumentException;
	public void beginUpdate();
	public void endUpdate();
	public void initialize();
	public void shutdown();
}
