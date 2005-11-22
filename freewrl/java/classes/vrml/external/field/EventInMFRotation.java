package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Node;
import vrml.external.Browser;

public class EventInMFRotation extends EventIn {

  public EventInMFRotation() { EventType = FieldTypes.MFROTATION; }

  public void          setValue(float[][] value) throws IllegalArgumentException {
        int count;
        if (value == null) {
                throw new IllegalArgumentException();
        }
        Browser.newSendEvent(this,  " " + value.length);
        for (count = 0; count < value.length; count++) {
                if (value[count].length < 4) {
                        throw new IllegalArgumentException();
                }
                Browser.newSendEvent(this,  " " + value[count][0] + " " + value[count][1] + " " + value[count][2] + " " + value[count][3]);
        }


    return;
  }

  public void          set1Value(int index, float[] value) throws IllegalArgumentException {
        if ((value == null) || (index < 0) || (value.length < 4)) {
                throw new IllegalArgumentException();
        }
        Browser.newSendEvent(this,  " " + index + " " + value[0] + " " + value[1] + " " + value[2] + " " + value[3]);
  return;
  }
}
