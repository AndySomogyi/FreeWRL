//AUTOMATICALLY GENERATED BY genfields.pl.
//DO NOT EDIT!!!!

package vrml.field;
import vrml.*;

public class ConstSFString extends ConstField {
    String s;

    public ConstSFString() {
    }
    public ConstSFString(String s) {
        this.s = s;
    }

    public String getValue() {
        __updateRead();
        return s;
    }

    public String toString() {
        __updateRead();
        return vrml.FWHelper.quote(s);
    }

    public void __fromPerl(String str) {
        s = FWHelper.base64decode(str);
    }

    public String __toPerl() {
        return FWHelper.base64encode(s);
    }
}