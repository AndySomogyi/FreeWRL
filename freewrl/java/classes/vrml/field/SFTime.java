//AUTOMATICALLY GENERATED BY genfields.pl.
//DO NOT EDIT!!!!

package vrml.field;
import vrml.*;
import java.io.BufferedReader;
import java.io.PrintWriter;
import java.io.IOException;

public class SFTime extends Field {
     double value;

    public SFTime() { }

    public SFTime(double value) {
	        this.value = value;
    }

    public double getValue() {
        __updateRead();
        return value;
    }

    public void setValue(double value) {
        this.value = value;
        __updateWrite();
    }


    public void setValue(ConstSFTime sfTime) {
        sfTime.__updateRead();
        value = sfTime.value;
        __updateWrite();
    }

    public void setValue(SFTime sfTime) {
        sfTime.__updateRead();
        value = sfTime.value;
        __updateWrite();
    }


    public String toString() {
        __updateRead();
        return String.valueOf(value);
    }

    public void __fromPerl(BufferedReader in)  throws IOException {
        
	//System.out.println ("fromPerl, Time");
		value = Double.parseDouble(in.readLine());
    }

    public void __toPerl(PrintWriter out)  throws IOException {
        out.print(value);
	//out.println();
    }
    //public void setOffset(String offs) { this.offset = offs; } //JAS2
    //public String getOffset() { return this.offset; } //JAS2
}