package sai;
import org.web3d.x3d.sai.*;
import java.util.*;

public class FWMFFloat extends FreeWRLMField implements MFFloat {
	FreeWRLBrowser browser;
	private static final int ROWS = 1;

	public FWMFFloat(FreeWRLFieldDefinition def, FreeWRLBrowser b) {
		super(def, b);
		browser = b;
	}

	public void getValue(float[] value)  throws ArrayIndexOutOfBoundsException {
		int lines;
		int count1;
		int count2;
		StringTokenizer tokens;
		String rep;

		if (RLreturn == null) {
			rep = browser.SendEventOut(nodePtr, offset, datasize, dataType, command);
		} else {
			rep = RLreturn;
		}

		tokens = new StringTokenizer(rep);

		lines = Integer.valueOf(tokens.nextToken()).intValue();

		if (value.length < lines) {
			throw new ArrayIndexOutOfBoundsException("MFFloat getValue passed array of insufficient size");
		}

		for (count1=0; count1<lines; count1++) {
			value[count1] = Float.valueOf(tokens.nextToken()).floatValue();
		}
	}

	public float get1Value(int index) throws ArrayIndexOutOfBoundsException {
		int lines;
		int count1;
		int count2;
		StringTokenizer tokens;
		String rep;
		float[] rval;

		if (RLreturn == null) {
			rep = browser.SendEventOut(nodePtr, offset, datasize, dataType, command);
		} else {
			rep = RLreturn;
		}

		tokens = new StringTokenizer(rep);

		lines = Integer.valueOf(tokens.nextToken()).intValue();
		
		rval = new float[lines];

		if (index > lines) {
			throw new ArrayIndexOutOfBoundsException("MFFloat get1Value passed index out of bounds");
		}

		for (count1=0; count1<lines; count1++) {
			rval[count1] = Float.valueOf(tokens.nextToken()).floatValue();
		}

		
		return rval[index];
	}
	public void setValue(int size, float[] value) {
		String val;
		int count;
		
		if (size > value.length) {
			size = value.length;
		}

		val = " " + size;

		for (count = 0; count < size; count++) {
			val = val + " " + value[count];
		}
		browser.newSendEvent(this, val);
	}
	public void set1Value(int index, float value) throws ArrayIndexOutOfBoundsException {
		browser.newSendEvent(this, " ONEVAL " + index + " " + value);
	}
	public void append(float[] value){
                int lines;
		int append_size;
                int count1, count2;
                StringTokenizer tokens;
                String rep;
                String val;

                if (RLreturn == null) {
                        rep = browser.SendEventOut(nodePtr, offset, datasize, dataType, command);
                } else {
                        rep = RLreturn;
                }

                tokens = new StringTokenizer(rep);
                lines = Integer.valueOf(tokens.nextToken()).intValue();

		append_size = value.length;

                val = " " + (lines + append_size);

                for (count1 = 0; count1 < lines; count1++) {
                	val = val + tokens.nextToken();
                }

		for (count1 = 0; count1 < append_size; count1++) {
			val = val + " " + value[count1];
		}

		browser.newSendEvent(this, val);
	}
	public void insertValue(int index, float[] value) throws ArrayIndexOutOfBoundsException {
                int lines;
                int insert_size;
                int count1, count2;
                StringTokenizer tokens;
                String rep;
                String val;

                if (RLreturn == null) {
                        rep = browser.SendEventOut(nodePtr, offset, datasize, dataType, command);
                } else {
                        rep = RLreturn;
                }

                tokens = new StringTokenizer(rep);
                lines = Integer.valueOf(tokens.nextToken()).intValue();

                insert_size = value.length;

		if ((index > lines) || (index < 0)) {
			throw new ArrayIndexOutOfBoundsException("MFFloat insertValue passed index out of bounds");
		}

                val = " " + (lines + insert_size);

                for (count1 = 0; count1 < index; count1++) {
                        val = val + tokens.nextToken();
                }

                for (count1 = 0; count1 < insert_size; count1++) {
                        val = val + " " + value[count1];
                }

		for (count1 = index; count1 < lines; count1++) {
			val = val + tokens.nextToken();
		}

                browser.newSendEvent(this, val);
	}
}
