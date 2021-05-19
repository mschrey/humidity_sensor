/*
   EEPROM Write

   Stores values read from analog input 0 into the EEPROM.
   These values will stay in the EEPROM when the board is
   turned off and may be retrieved later by another sketch.
*/

#include <EEPROM.h>

// the current address in the EEPROM (i.e. which byte
// we're going to write to next)
int addr = 0;

void setup() {
  EEPROM.begin(512);
  Serial.begin(115200);

  double adc_coefficient = 3.978;
  EEPROM.put(0, 0xBADEAFFE);
  delay(100);
  EEPROM.put(4, adc_coefficient);
  // advance to the next address.  there are 512 bytes in
  // the EEPROM, so go back to 0 when we hit 512.
  // save all changes to the flash.
  int ret = EEPROM.commit();
  delay(500);
  if (ret) {
    Serial.println("EEPROM successfully committed");
  } else {
    Serial.println("ERROR! EEPROM commit failed");
  }  
}

void loop() {
     
}
