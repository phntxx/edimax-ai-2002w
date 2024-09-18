#include <Arduino.h>
#include <Wire.h>
#include <Math.h>

#define SHT30_ADDRESS 0x44

class SHT30
{

private:
  uint8_t _addr;
  TwoWire *_wire = NULL;
  float _t, _h;
  bool _handleError();

public:
  SHT30(uint8_t addr = SHT30_ADDRESS, TwoWire *wire = &Wire);
  bool begin();
  bool measure();
  void setHeater(bool state);
  float temperature();
  float humidity();
};