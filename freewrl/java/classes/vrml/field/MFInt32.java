//AUTOMATICALLY GENERATED BY genfields.pl.
//DO NOT EDIT!!!!

package vrml.field;
import vrml.*;
import java.io.BufferedReader;
import java.io.PrintWriter;
import java.io.IOException;

public class MFInt32 extends MField {
    public MFInt32() {
    }

    public MFInt32(int[] value) {
        this(value.length, value);
    }

    public MFInt32(int size, int[] value) {
        for (int i = 0; i < size; i++)	
            __vect.addElement(new ConstSFInt32(value[i]));
    }
	
    public void getValue(int[] value) {
        __updateRead();
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            ConstSFInt32 sfInt32 = (ConstSFInt32) __vect.elementAt(i);
            value[i] = sfInt32.value;
        }
    }

    public int get1Value(int index) {
        __update1Read(index);
        return ((ConstSFInt32) __vect.elementAt(index)).getValue();
    }

    public void setValue(int[] value) {
        setValue(value.length, value);
    }

    public void setValue(int size, int[] value) {
        __vect.clear();
        for (int i = 0; i < size; i++)
            __vect.addElement(new ConstSFInt32(value[i]));
        __updateWrite();
    }

    public void set1Value(int index, int value) {
        __set1Value(index, new ConstSFInt32(value));
    }

    public void set1Value(int index, SFInt32 sfInt32) {
        sfInt32.__updateRead();
        __set1Value(index, new ConstSFInt32(sfInt32.value));
    }

    public void set1Value(int index, ConstSFInt32 sfInt32) {
        __set1Value(index, sfInt32);
    }

    public void addValue(int value) {
        __addValue(new ConstSFInt32(value));
    }

    public void addValue(SFInt32 sfInt32) {
        sfInt32.__updateRead();
        __addValue(new ConstSFInt32(sfInt32.value));
    }

    public void addValue(ConstSFInt32 sfInt32) {
        __addValue(sfInt32);
    }

    public void insertValue(int index, int value) {
        __insertValue(index, new ConstSFInt32(value));
    }

    public void insertValue(int index, SFInt32 sfInt32) {
        sfInt32.__updateRead();
        __insertValue(index, new ConstSFInt32(sfInt32.value));
    }

    public void insertValue(int index, ConstSFInt32 sfInt32) {
        __insertValue(index, sfInt32);
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
            ConstSFInt32 sf = new ConstSFInt32();
            sf.__fromPerl(in);
            __vect.addElement(sf);
        }
    }

    public void __toPerl(PrintWriter out)  throws IOException {
        StringBuffer sb = new StringBuffer("");
        int size = __vect.size();
	//out.print(size);
        for (int i = 0; i < size; i++) {
            ((ConstSFInt32) __vect.elementAt(i)).__toPerl(out);
	    if (i != (size-1)) out.print (", ");
	}
	//out.println();
    }
    //public void setOffset(String offs) { this.offset = offs; } //JAS2
    //public String getOffset() { return this.offset; } //JAS2
}