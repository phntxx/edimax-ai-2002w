#include "vz-89te.h"

VZ_89TE::VZ_89TE(uint8_t addr, TwoWire *wire) : _addr(addr), _wire(wire)
{
  // ...
}

bool VZ_89TE::_handleError()
{
  this->_c = 0;
  this->_v = 0;
  this->_s = 0;
  return false;
}

bool VZ_89TE::begin()
{
  return true;
}

bool VZ_89TE::measure()
{

  static uint8_t rx_bytes[7];

  this->_wire->beginTransmission(this->_addr);
  this->_wire->write(0x0C);
  this->_wire->write(0x00);
  this->_wire->write(0x00);
  this->_wire->write(0x00);
  this->_wire->write(0x00);
  this->_wire->write(0x00);

  if (this->_wire->endTransmission(false) != 0)
  {
    return this->_handleError();
  }

  this->_wire->beginTransmission(0xE1);
  this->_wire->endTransmission();

  if (this->_wire->requestFrom(this->_addr, (uint8_t)7) != 7)
  {
    return this->_handleError();
  }

  for (int i = 0; i < 7; i++)
  {
    rx_bytes[i] = Wire.read();
  }

  this->_c = (rx_bytes[1] - 13) * (1600.0 / 229) + 400;
  this->_v = (rx_bytes[0] - 13) * (1000.0 / 229);
  this->_s = rx_bytes[5];

  return true;
}

float VZ_89TE::co2()
{
  return this->_c;
}

float VZ_89TE::voc()
{
  return this->_v;
}

uint8_t VZ_89TE::status()
{
  return this->_s;
}