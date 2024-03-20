#include "pms5003.h"

PMS5003::PMS5003(int rx, int tx, int baud) : SoftwareSerial(rx, tx), _baud(baud)
{
  // ...
}

bool PMS5003::_valid_checksum(uint8_t data[])
{
  uint16_t checksum = this->_word(data[30], data[31]);

  uint16_t check = data[0] + data[1];
  for (int i = 3; i < 30; i += 2)
  {
    check += data[i];
  }

  return check == checksum;
}

bool PMS5003::_handleError()
{
  this->_sp_pm10 = 0;
  this->_sp_pm25 = 0;
  this->_sp_pm100 = 0;
  this->_ae_pm10 = 0;
  this->_ae_pm25 = 0;
  this->_ae_pm100 = 0;
  this->_p03 = 0;
  this->_p05 = 0;
  this->_p10 = 0;
  this->_p25 = 0;
  this->_p50 = 0;
  this->_p100 = 0;
  return false;
}

uint16_t PMS5003::_word(uint8_t high, uint8_t low)
{
  return (high << 8) + low;
}

bool PMS5003::begin()
{
  SoftwareSerial::begin(this->_baud);

  // set passive mode
  uint8_t command[] = {0x42, 0x4D, 0xE1, 0x00, 0x00, 0x01, 0x70};
  write(command, sizeof(command));
  return true;
}

bool PMS5003::measure()
{

  uint8_t command[] = {0x42, 0x4D, 0xE2, 0x00, 0x00, 0x01, 0x71};
  write(command, sizeof(command));

  uint8_t data[32];
  if (available())
  {
    readBytes(data, 32);
  }

  if ((data[0] != 0x42) || (data[1] != 0x4d))
  {
    return this->_handleError();
  }

  if (!_valid_checksum(data))
  {
    return this->_handleError();
  }

  this->_sp_pm10 = this->_word(data[4], data[5]);
  this->_sp_pm25 = this->_word(data[6], data[7]);
  this->_sp_pm100 = this->_word(data[8], data[9]);

  this->_ae_pm10 = this->_word(data[10], data[11]);
  this->_ae_pm25 = this->_word(data[12], data[13]);
  this->_ae_pm100 = this->_word(data[14], data[15]);

  this->_p03 = this->_word(data[16], data[17]);
  this->_p05 = this->_word(data[18], data[19]);
  this->_p10 = this->_word(data[20], data[21]);
  this->_p25 = this->_word(data[22], data[23]);
  this->_p50 = this->_word(data[24], data[25]);
  this->_p100 = this->_word(data[26], data[27]);

  return true;
}

void PMS5003::wakeUp()
{
  uint8_t command[] = {0x42, 0x4D, 0xE4, 0x00, 0x01, 0x01, 0x74};
  write(command, sizeof(command));

  delay(5000);
}

void PMS5003::sleep()
{
  uint8_t command[] = {0x42, 0x4D, 0xE4, 0x00, 0x00, 0x01, 0x73};
  write(command, sizeof(command));
}

uint16_t PMS5003::ae_pm10()
{
  return this->_ae_pm10;
}

uint16_t PMS5003::ae_pm25()
{
  return this->_ae_pm25;
}

uint16_t PMS5003::ae_pm100()
{
  return this->_ae_pm100;
}

uint16_t PMS5003::sp_pm10()
{
  return this->_sp_pm10;
}

uint16_t PMS5003::sp_pm25()
{
  return this->_sp_pm25;
}

uint16_t PMS5003::sp_pm100()
{
  return this->_sp_pm100;
}

uint16_t PMS5003::p03()
{
  return this->_p03;
}

uint16_t PMS5003::p05()
{
  return this->_p05;
}

uint16_t PMS5003::p10()
{
  return this->_p10;
}

uint16_t PMS5003::p25()
{
  return this->_p25;
}

uint16_t PMS5003::p50()
{
  return this->_p50;
}

uint16_t PMS5003::p100()
{
  return this->_p100;
}