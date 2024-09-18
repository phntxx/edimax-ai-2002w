#include <Arduino.h>
#include <functional>
#include "IPAddress.h"
#include "Client.h"
#include "Stream.h"

#define MQTT_VERSION_3_1_1 4
#define MQTT_VERSION MQTT_VERSION_3_1_1

// MQTT_MAX_PACKET_SIZE : Maximum packet size. Override with setBufferSize().
#define MQTT_MAX_PACKET_SIZE 256

// MQTT_KEEPALIVE : keepAlive interval in Seconds. Override with setKeepAlive()
#define MQTT_KEEPALIVE 15

// MQTT_SOCKET_TIMEOUT: socket timeout interval in Seconds. Override with setSocketTimeout()
#define MQTT_SOCKET_TIMEOUT 15

// Possible values for client.state()
#define MQTT_CONNECTION_TIMEOUT -4
#define MQTT_CONNECTION_LOST -3
#define MQTT_CONNECT_FAILED -2
#define MQTT_DISCONNECTED -1
#define MQTT_CONNECTED 0
#define MQTT_CONNECT_BAD_PROTOCOL 1
#define MQTT_CONNECT_BAD_CLIENT_ID 2
#define MQTT_CONNECT_UNAVAILABLE 3
#define MQTT_CONNECT_BAD_CREDENTIALS 4
#define MQTT_CONNECT_UNAUTHORIZED 5

#define MQTTCONNECT 1 << 4     // Client request to connect to Server
#define MQTTCONNACK 2 << 4     // Connect Acknowledgment
#define MQTTPUBLISH 3 << 4     // Publish message
#define MQTTPUBACK 4 << 4      // Publish Acknowledgment
#define MQTTPUBREC 5 << 4      // Publish Received (assured delivery part 1)
#define MQTTPUBREL 6 << 4      // Publish Release (assured delivery part 2)
#define MQTTPUBCOMP 7 << 4     // Publish Complete (assured delivery part 3)
#define MQTTSUBACK 9 << 4      // Subscribe Acknowledgment
#define MQTTPINGREQ 12 << 4    // PING Request
#define MQTTPINGRESP 13 << 4   // PING Response
#define MQTTDISCONNECT 14 << 4 // Client is Disconnecting
#define MQTTReserved 15 << 4   // Reserved

#define MQTTQOS0 (0 << 1)
#define MQTTQOS1 (1 << 1)
#define MQTTQOS2 (2 << 1)

// Maximum size of fixed header and variable length size header
#define MQTT_MAX_HEADER_SIZE 5

#define CHECK_STRING_LENGTH(l, s)                              \
  if (l + 2 + strnlen(s, this->bufferSize) > this->bufferSize) \
  {                                                            \
    _client->stop();                                           \
    return false;                                              \
  }

class MQTT : public Print
{
private:
  Client *_client;
  uint8_t *buffer;
  uint16_t bufferSize;
  uint16_t keepAlive;
  uint16_t socketTimeout;
  uint16_t nextMsgId;
  unsigned long lastOutActivity;
  unsigned long lastInActivity;
  bool pingOutstanding;
  uint32_t readPacket(uint8_t *);
  boolean readByte(uint8_t *result);
  boolean readByte(uint8_t *result, uint16_t *index);
  boolean write(uint8_t header, uint8_t *buf, uint16_t length);
  uint16_t writeString(const char *string, uint8_t *buf, uint16_t pos);
  size_t buildHeader(uint8_t header, uint8_t *buf, uint16_t length);
  IPAddress ip;
  String domain;
  uint16_t port;
  Stream *stream;
  int _state;
  boolean publish(const char *topic, const uint8_t *payload, unsigned int plength, boolean retained);

public:
  MQTT();
  MQTT(IPAddress, uint16_t, Client &client);
  MQTT(String, uint16_t, Client &client);

  ~MQTT();

  MQTT &setServer(IPAddress ip, uint16_t port);
  MQTT &setServer(String domain, uint16_t port);
  MQTT &setClient(Client &client);
  MQTT &setStream(Stream &stream);
  MQTT &setKeepAlive(uint16_t keepAlive);
  MQTT &setSocketTimeout(uint16_t timeout);

  boolean setBufferSize(uint16_t size);
  uint16_t getBufferSize();

  boolean connect(String id);
  boolean connect(String id, String user, String pass);
  boolean connect(const char *id, const char *user, const char *pass, const char *willTopic, uint8_t willQos, boolean willRetain, const char *willMessage, boolean cleanSession);

  void reconnect(String id);
  void reconnect(String id, String user, String pass);
  void disconnect();

  boolean publish(const char *topic, const char *payload);
  boolean publish(String topic, uint16_t payload);
  boolean publish(String topic, float payload);

  virtual size_t write(uint8_t);
  virtual size_t write(const uint8_t *buffer, size_t size);

  boolean loop();
  boolean connected();
  int state();
};