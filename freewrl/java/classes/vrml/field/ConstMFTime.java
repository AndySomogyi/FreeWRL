//AUTOMATICALLY GENERATED BY genfields.pl.
//DO NOT EDIT!!!!

package vrml.field;
import vrml.*;
import java.util.StringTokenizer;

public class ConstMFTime extends ConstMField {
    public ConstMFTime() {
    }

    public ConstMFTime(double[] value) {
        this(value.length, value);
    }

    public ConstMFTime(int size, double[] value) {
        for (int i = 0; i < size; i++)	
            __vect.addElement(new ConstSFTime(value[i]));
    }
	
    public void getValue(double[] value) {
        __updateRead();
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            ConstSFTime sfTime = (ConstSFTime) __vect.elementAt(i);
            value[i] = sfTime.value;
        }
    }

    public double get1Value(int index) {
        __update1Read(index);
        return ((ConstSFTime) __vect.elementAt(index)).getValue();
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

    public void __fromPerl(String str) {
        __vect.clear();
        StringTokenizer st = new StringTokenizer(str,",");
        while (st.hasMoreTokens()) {
            ConstSFTime sf = new ConstSFTime();
            sf.__fromPerl(st.nextToken());
            __vect.addElement(sf);
        }
    }

    public String __toPerl() {
        StringBuffer sb = new StringBuffer("");
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            if (i > 0) sb.append(",");
            sb.append(((ConstSFTime) __vect.elementAt(i)).__toPerl());
        }
        return sb.append("").toString();
    }
}