# humidity_sensor
Network of Humidity Sensors - This is the sensor node code

This sketch measures humidity and temperature using a DHT22 sensor. It will send this data together with a sensor ID and its battery voltage to a "Humidity Hub". The sensor ID is read from four input pins ID0 ... ID3, while the battery voltage is read using the ADC. Once the data is sent, the ESP8266 will go to sleep for 10 minutes before it repeats this cycle. 

If the ESP8266 is unable to reach the Humidity Hub, it will go to sleep for 1 minute only and then retry sending the data. 

## Correction Value in EEPROM ##

The sketch feuchtesensor.ino assumes a certain magic value and a correction value to be present in the EEPROM of the ESP8266. This corrects for variation in the ADC performance and the external resistor-based voltage divider. 

Please use the eeprom_write.ino sketch to write the magic value and the correction value to the correction location in EEPROM.

## WLAN Credentials ##
Please set your WLAN credentials (SSID, Password and IP of Humidity Hub) in lines 35ff. 
