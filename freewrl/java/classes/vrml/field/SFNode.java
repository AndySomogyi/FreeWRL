//AUTOMATICALLY GENERATED BY genfields.pl.
//DO NOT EDIT!!!!

package vrml.field;
import vrml.*;
import java.io.BufferedReader;
import java.io.PrintWriter;
import java.io.IOException;

public class SFNode extends Field {
     BaseNode node;

    public SFNode() { }

    public SFNode(BaseNode node) {
	        this.node = node;
    }

    public BaseNode getValue() {
        __updateRead();
        return node;
    }

    public void setValue(BaseNode node) {
        this.node = node;
        __updateWrite();
    }


    public void setValue(ConstSFNode sfNode) {
        sfNode.__updateRead();
        node = sfNode.node;
        __updateWrite();
    }

    public void setValue(SFNode sfNode) {
        sfNode.__updateRead();
        node = sfNode.node;
        __updateWrite();
    }


    public String toString() {
        __updateRead();
        return FWHelper.nodeToString(node);
    }

    public void __fromPerl(BufferedReader in)  throws IOException {
        
	//System.out.println ("fromPerl, Node");
		node = new vrml.node.Node(in.readLine());
    }

    public void __toPerl(PrintWriter out)  throws IOException {
        out.print(node._get_nodeid());
	//out.println();
    }
    //public void setOffset(String offs) { this.offset = offs; } //JAS2
    //public String getOffset() { return this.offset; } //JAS2
}