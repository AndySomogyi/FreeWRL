//AUTOMATICALLY GENERATED BY genfields.pl.
//DO NOT EDIT!!!!

package vrml.field;
import vrml.*;
import java.io.BufferedReader;
import java.io.PrintWriter;
import java.io.IOException;

public class ConstMFVec2f extends ConstMField {
    public ConstMFVec2f() {
    }

    public ConstMFVec2f(float[] vec2fs) {
        this(vec2fs.length, vec2fs);
    }

    public ConstMFVec2f(int size, float[] vec2fs) {
        for (int i = 0; i < size; i += 2)	
            __vect.addElement(new ConstSFVec2f(vec2fs[i], vec2fs[i+1]));
    }

    public ConstMFVec2f(float[][] vec2fs) {
        for (int i = 0; i < vec2fs.length; i++)
            __vect.addElement(new ConstSFVec2f(vec2fs[i][0], vec2fs[i][1]));
    }
	
    public void getValue(float[] vec2fs) {
        __updateRead();
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            ConstSFVec2f sfVec2f = (ConstSFVec2f) __vect.elementAt(i);
            vec2fs[2*i+0] = sfVec2f.x;
            vec2fs[2*i+1] = sfVec2f.y;
        }
    }

    public void getValue(float[][] vec2fs) {
        __updateRead();
        int size = __vect.size();
        for (int i = 0; i < size; i++)
            ((ConstSFVec2f) __vect.elementAt(i)).getValue(vec2fs[i]);
    }

    public void get1Value(int index, float[] vec2fs) {
        __update1Read(index);
        ((ConstSFVec2f) __vect.elementAt(index)).getValue(vec2fs);
    }

    public void get1Value(int index, SFVec2f sfVec2f) {
        __update1Read(index);
        sfVec2f.setValue((ConstSFVec2f) __vect.elementAt(index));
    }

    public String toString() {
        __updateRead();
        StringBuffer sb = new StringBuffer("[");
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            if (i > 0) sb.append(", ");
            sb.append(__vect.elementAt(i));
        }
        return sb.append("]").toString();
    }

    public void __fromPerl(BufferedReader in)  throws IOException {
        __vect.clear();
	String lenline = in.readLine();
	//System.out.println ("__fromPerl, read in length as " + lenline);
        //int len = Integer.parseInt(in.readLine());
	int len = Integer.parseInt(lenline);
        for (int i = 0; i < len; i++) {
            ConstSFVec2f sf = new ConstSFVec2f();
            sf.__fromPerl(in);
            __vect.addElement(sf);
        }
    }

    public void __toPerl(PrintWriter out)  throws IOException {
        StringBuffer sb = new StringBuffer("");
        int size = __vect.size();
	//out.print(size);
        for (int i = 0; i < size; i++) {
            ((ConstSFVec2f) __vect.elementAt(i)).__toPerl(out);
	    if (i != (size-1)) out.print (", ");
	}
	//out.println();
    }
    //public void setOffset(String offs) { this.offset = offs; } //JAS2
    //public String getOffset() { return this.offset; } //JAS2
}