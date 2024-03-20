#include "sht30.h"

SHT30::SHT30(uint8_t addr, TwoWire *wire) : _addr(addr), _wire(wire)
{
  // ...
}

bool SHT30::_handleError()
{
  this->_t = 0;
  this->_h = 0;
  return false;
}

bool SHT30::begin()
{
  this->_wire->beginTransmission(this->_addr);
  this->_wire->write(0x27);
  this->_wire->write(0x37);
  return this->_wire->endTransmission() == 0;
}

bool SHT30::measure()
{

  uint8_t rx_bytes[6];
  memset(rx_bytes, 0, 6);

  this->_wire->beginTransmission(this->_addr);
  this->_wire->write(0xE0);
  this->_wire->write(0x00);

  if (this->_wire->endTransmission(false) != 0)
  {
    return this->_handleError();
  }

  if (this->_wire->requestFrom(this->_addr, (uint8_t)6) != 6)
  {
    return this->_handleError();
  }

  this->_wire->readBytes(rx_bytes, 6);

  uint16_t t_ticks = ((uint16_t)rx_bytes[0] << 8) | rx_bytes[1];
  uint16_t h_ticks = ((uint16_t)rx_bytes[3] << 8) | rx_bytes[4];

  this->_t = min(-45.0f + (175.0f * (t_ticks / 65535.0f)), (float)100);
  this->_h = max(100 * (h_ticks / 65535.0f), (float)0);

  return true;
}

float SHT30::temperature()
{
  return this->_t;
}

float SHT30::humidity()
{
  return this->_h;
}