package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Browser;

public class EventInMFString extends EventIn {
	public EventInMFString() { EventType = FieldTypes.MFSTRING; }

	public void          setValue(String[] value) {
		int count;
		String sestr;

		// start off the return value with the number of elements:
		sestr = ""+value.length+" ";
		for (count = 0; count < value.length; count++) {
			sestr = sestr+" "+count+";" + value[count].length()+
					":" + value[count] + " ";
		}
		Browser.newSendEvent (this, sestr);
	}

	public void          set1Value(int index, String value) {
		// send index, and -1, indicating that we don't know
		// the total size of this array.
		System.out.println ("Warning - EventInMFString - set1Value might not work");
		Browser.newSendEvent (this, ""+index+1 +" "+
				+index+";" + value.length() +
					":" + value + " ");
	}
}
