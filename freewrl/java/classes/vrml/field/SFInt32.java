// GENERATED BY genfields.pl. DO NOT EDIT!
package vrml.field;
import vrml.*;
import java.util.*;

public class SFInt32 extends Field {
int v;
public SFInt32() { v = 0;}
public SFInt32(int val) { v=val;}
public void setValue(int val) {v=val; value_touched();}
public SFInt32(String s) throws Exception {
		;
		if(s == null) {
			v = 0;; return;
		}
		s = s.trim();
		
	s = s.trim();
	v = new Integer(s).intValue();

	}public int getValue() {return v;}
public void setValue(ConstSFInt32 f) {v = f.getValue(); value_touched();}
		public void setValue(SFInt32 f) {v = f.getValue(); value_touched(); }
public String toString() {return new Integer(v).toString();}public Object clone() {SFInt32 _x = new SFInt32(v); return _x;}}