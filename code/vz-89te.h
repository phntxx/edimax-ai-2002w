#include <Arduino.h>
#include <Wire.h>

#define VZ_89TE_ADDRESS 0x70

class VZ_89TE
{

private:
  uint8_t _addr;
  TwoWire *_wire = NULL;
  float _c, _v, _s;
  bool _handleError();

public:
  VZ_89TE(uint8_t addr = VZ_89TE_ADDRESS, TwoWire *wire = &Wire);
  bool begin();
  bool measure();
  float co2();
  float voc();
  uint8_t status();
};