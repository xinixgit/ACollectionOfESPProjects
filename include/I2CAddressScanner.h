#include <Wire.h>

#define WIRE Wire

struct I2CAddressScanner
{
  void scan()
  {
    WIRE.begin();
    for (int address = 1; address < 127; address++)
    {
      // The i2c_scanner uses the return value of
      // the Write.endTransmisstion to see if
      // a device did acknowledge to the address.
      WIRE.beginTransmission(address);
      uint8_t error = WIRE.endTransmission();

      if (error == 0)
      {
        Serial.print("I2C device found at address 0x");
        if (address < 16)
          Serial.print("0");
        Serial.print(address, HEX);
        Serial.println("  !");
      }
      else if (error == 4)
      {
        Serial.print("Unknown error at address 0x");
        if (address < 16)
          Serial.print("0");
        Serial.println(address, HEX);
      }
    }
  }
};
