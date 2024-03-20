#include <Arduino.h>
#include <SoftwareSerial.h>

#define BAUD_RATE 9600

class PMS5003 : public SoftwareSerial
{

private:
  int _baud;
  uint16_t _sp_pm10, _sp_pm25, _sp_pm100;
  uint16_t _ae_pm10, _ae_pm25, _ae_pm100;
  uint16_t _p03, _p05, _p10, _p25, _p50, _p100;
  bool _handleError();
  bool _valid_checksum(uint8_t data[]);
  uint16_t _word(uint8_t high, uint8_t low);

public:
  PMS5003(int rx, int tx, int baud = BAUD_RATE);
  bool begin();
  bool measure();

  void wakeUp();
  void sleep();

  uint16_t ae_pm10();
  uint16_t ae_pm25();
  uint16_t ae_pm100();

  uint16_t sp_pm10();
  uint16_t sp_pm25();
  uint16_t sp_pm100();

  uint16_t p03();
  uint16_t p05();
  uint16_t p10();
  uint16_t p25();
  uint16_t p50();
  uint16_t p100();
};