package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Browser;

public class EventInSFColor extends EventIn {

  public EventInSFColor() { EventType = FieldTypes.SFCOLOR; }

  public void          setValue(float[] value) throws
	IllegalArgumentException {

        Browser.SendEvent (inNode , command, "" + value[0] + " " + value[1] +
                  " " + value[2]);
    return;
  }
}
