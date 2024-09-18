#include "mqtt.h"

MQTT::MQTT()
{
    this->_state = MQTT_DISCONNECTED;
    this->stream = NULL;
    this->bufferSize = 0;
    setBufferSize(MQTT_MAX_PACKET_SIZE);
    setKeepAlive(MQTT_KEEPALIVE);
    setSocketTimeout(MQTT_SOCKET_TIMEOUT);
}

MQTT::MQTT(IPAddress addr, uint16_t port, Client &client)
{
    this->_state = MQTT_DISCONNECTED;
    setServer(addr, port);
    setClient(client);
    this->stream = NULL;
    this->bufferSize = 0;
    setBufferSize(MQTT_MAX_PACKET_SIZE);
    setKeepAlive(MQTT_KEEPALIVE);
    setSocketTimeout(MQTT_SOCKET_TIMEOUT);
}

MQTT::MQTT(String domain, uint16_t port, Client &client)
{
    this->_state = MQTT_DISCONNECTED;
    setServer(domain, port);
    setClient(client);

    this->stream = NULL;
    this->bufferSize = 0;
    setBufferSize(MQTT_MAX_PACKET_SIZE);
    setKeepAlive(MQTT_KEEPALIVE);
    setSocketTimeout(MQTT_SOCKET_TIMEOUT);
}

MQTT::~MQTT()
{
    free(this->buffer);
}

boolean MQTT::connect(String id)
{
    return connect(id.c_str(), NULL, NULL, 0, 0, 0, 0, 1);
}

boolean MQTT::connect(String id, String user, String pass)
{
    return connect(id.c_str(), user.c_str(), pass.c_str(), 0, 0, 0, 0, 1);
}

boolean MQTT::connect(const char *id, const char *user, const char *pass, const char *willTopic, uint8_t willQos, boolean willRetain, const char *willMessage, boolean cleanSession)
{
    if (!connected())
    {
        int result = 0;

        if (_client->connected())
        {
            result = 1;
        }
        else
        {
            if (domain != "")
            {
                result = _client->connect(this->domain.c_str(), this->port);
            }
            else
            {
                result = _client->connect(this->ip, this->port);
            }
        }

        if (result == 1)
        {
            nextMsgId = 1;
            // Leave room in the buffer for header and variable length field
            uint16_t length = MQTT_MAX_HEADER_SIZE;

            uint8_t d[7] = {0x00, 0x04, 'M', 'Q', 'T', 'T', MQTT_VERSION};
            for (unsigned int j = 0; j < 7; j++)
            {
                this->buffer[length++] = d[j];
            }

            uint8_t v;
            if (willTopic)
            {
                v = 0x04 | (willQos << 3) | (willRetain << 5);
            }
            else
            {
                v = 0x00;
            }
            if (cleanSession)
            {
                v = v | 0x02;
            }

            if (user != NULL)
            {
                v = v | 0x80;

                if (pass != NULL)
                {
                    v = v | (0x80 >> 1);
                }
            }
            this->buffer[length++] = v;

            this->buffer[length++] = ((this->keepAlive) >> 8);
            this->buffer[length++] = ((this->keepAlive) & 0xFF);

            CHECK_STRING_LENGTH(length, id)
            length = writeString(id, this->buffer, length);
            if (willTopic)
            {
                CHECK_STRING_LENGTH(length, willTopic)
                length = writeString(willTopic, this->buffer, length);
                CHECK_STRING_LENGTH(length, willMessage)
                length = writeString(willMessage, this->buffer, length);
            }

            if (user != NULL)
            {
                CHECK_STRING_LENGTH(length, user)
                length = writeString(user, this->buffer, length);
                if (pass != NULL)
                {
                    CHECK_STRING_LENGTH(length, pass)
                    length = writeString(pass, this->buffer, length);
                }
            }

            write(1 << 4, this->buffer, length - MQTT_MAX_HEADER_SIZE);

            lastInActivity = lastOutActivity = millis();

            while (!_client->available())
            {
                unsigned long t = millis();
                if (t - lastInActivity >= ((int32_t)this->socketTimeout * 1000UL))
                {
                    _state = MQTT_CONNECTION_TIMEOUT;
                    _client->stop();
                    return false;
                }
            }
            uint8_t llen;
            uint32_t len = readPacket(&llen);

            if (len == 4)
            {
                if (buffer[3] == 0)
                {
                    lastInActivity = millis();
                    pingOutstanding = false;
                    _state = MQTT_CONNECTED;
                    return true;
                }
                else
                {
                    _state = buffer[3];
                }
            }
            _client->stop();
        }
        else
        {
            _state = MQTT_CONNECT_FAILED;
        }
        return false;
    }
    return true;
}

// reads a byte into result
boolean MQTT::readByte(uint8_t *result)
{
    uint32_t previousMillis = millis();
    while (!_client->available())
    {
        yield();
        uint32_t currentMillis = millis();
        if (currentMillis - previousMillis >= ((int32_t)this->socketTimeout * 1000))
        {
            return false;
        }
    }
    *result = _client->read();
    return true;
}

// reads a byte into result[*index] and increments index
boolean MQTT::readByte(uint8_t *result, uint16_t *index)
{
    uint16_t current_index = *index;
    uint8_t *write_address = &(result[current_index]);
    if (readByte(write_address))
    {
        *index = current_index + 1;
        return true;
    }
    return false;
}

uint32_t MQTT::readPacket(uint8_t *lengthLength)
{
    uint16_t len = 0;
    if (!readByte(this->buffer, &len))
        return 0;
    bool isPublish = (this->buffer[0] & 0xF0) == MQTTPUBLISH;
    uint32_t multiplier = 1;
    uint32_t length = 0;
    uint8_t digit = 0;
    uint16_t skip = 0;
    uint32_t start = 0;

    do
    {
        if (len == 5)
        {
            // Invalid remaining length encoding - kill the connection
            _state = MQTT_DISCONNECTED;
            _client->stop();
            return 0;
        }
        if (!readByte(&digit))
            return 0;
        this->buffer[len++] = digit;
        length += (digit & 127) * multiplier;
        multiplier <<= 7; // multiplier *= 128
    } while ((digit & 128) != 0);
    *lengthLength = len - 1;

    if (isPublish)
    {
        // Read in topic length to calculate bytes to skip over for Stream writing
        if (!readByte(this->buffer, &len))
            return 0;
        if (!readByte(this->buffer, &len))
            return 0;
        skip = (this->buffer[*lengthLength + 1] << 8) + this->buffer[*lengthLength + 2];
        start = 2;
        if (this->buffer[0] & MQTTQOS1)
        {
            // skip message id
            skip += 2;
        }
    }
    uint32_t idx = len;

    for (uint32_t i = start; i < length; i++)
    {
        if (!readByte(&digit))
            return 0;
        if (this->stream)
        {
            if (isPublish && idx - *lengthLength - 2 > skip)
            {
                this->stream->write(digit);
            }
        }

        if (len < this->bufferSize)
        {
            this->buffer[len] = digit;
            len++;
        }
        idx++;
    }

    if (!this->stream && idx > this->bufferSize)
    {
        len = 0; // This will cause the packet to be ignored.
    }
    return len;
}

boolean MQTT::loop()
{
    if (connected())
    {
        unsigned long t = millis();
        if ((t - lastInActivity > this->keepAlive * 1000UL) || (t - lastOutActivity > this->keepAlive * 1000UL))
        {
            if (pingOutstanding)
            {
                this->_state = MQTT_CONNECTION_TIMEOUT;
                _client->stop();
                return false;
            }
            else
            {
                this->buffer[0] = MQTTPINGREQ;
                this->buffer[1] = 0;
                _client->write(this->buffer, 2);
                lastOutActivity = t;
                lastInActivity = t;
                pingOutstanding = true;
            }
        }
        if (_client->available())
        {
            uint8_t llen;
            if (readPacket(&llen) > 0)
            {
                lastInActivity = t;
                uint8_t type = this->buffer[0] & 0xF0;
                if (type == MQTTPINGREQ)
                {
                    this->buffer[0] = MQTTPINGRESP;
                    this->buffer[1] = 0;
                    _client->write(this->buffer, 2);
                }
                else if (type == MQTTPINGRESP)
                {
                    pingOutstanding = false;
                }
            }
            else if (!connected())
            {
                return false;
            }
        }
        return true;
    }
    return false;
}

boolean MQTT::publish(const char *topic, const char *payload)
{
    return publish(topic, (const uint8_t *)payload, payload ? strnlen(payload, this->bufferSize) : 0, false);
}

boolean MQTT::publish(String topic, uint16_t payload)
{
    String payloadString = String(payload);
    return publish(topic.c_str(), (const uint8_t *)payloadString.c_str(), payload ? payloadString.length() : 0, false);
}

boolean MQTT::publish(String topic, float payload)
{
    String payloadString = String(payload);
    return publish(topic.c_str(), (const uint8_t *)payloadString.c_str(), payload ? payloadString.length() : 0, false);
}

boolean MQTT::publish(const char *topic, const uint8_t *payload, unsigned int plength, boolean retained)
{
    if (!connected())
    {
        return false;
    }

    if (this->bufferSize < MQTT_MAX_HEADER_SIZE + 2 + strnlen(topic, this->bufferSize) + plength)
    {
        // Too long
        return false;
    }

    // Leave room in the buffer for header and variable length field
    uint16_t length = MQTT_MAX_HEADER_SIZE;
    length = writeString(topic, this->buffer, length);

    // Add payload
    for (uint16_t i = 0; i < plength; i++)
    {
        this->buffer[length++] = payload[i];
    }

    // Write the header
    uint8_t header = MQTTPUBLISH;
    if (retained)
    {
        header |= 1;
    }
    return write(header, this->buffer, length - MQTT_MAX_HEADER_SIZE);
}

size_t MQTT::buildHeader(uint8_t header, uint8_t *buf, uint16_t length)
{
    uint8_t lenBuf[4];
    uint8_t llen = 0;
    uint8_t digit;
    uint8_t pos = 0;
    uint16_t len = length;
    do
    {

        digit = len & 127; // digit = len %128
        len >>= 7;         // len = len / 128
        if (len > 0)
        {
            digit |= 0x80;
        }
        lenBuf[pos++] = digit;
        llen++;
    } while (len > 0);

    buf[4 - llen] = header;
    for (int i = 0; i < llen; i++)
    {
        buf[MQTT_MAX_HEADER_SIZE - llen + i] = lenBuf[i];
    }
    return llen + 1; // Full header size is variable length bit plus the 1-byte fixed header
}

size_t MQTT::write(uint8_t data)
{
    lastOutActivity = millis();
    return _client->write(data);
}

size_t MQTT::write(const uint8_t *buffer, size_t size)
{
    lastOutActivity = millis();
    return _client->write(buffer, size);
}

boolean MQTT::write(uint8_t header, uint8_t *buf, uint16_t length)
{
    uint16_t rc;
    uint8_t hlen = buildHeader(header, buf, length);

    rc = _client->write(buf + (MQTT_MAX_HEADER_SIZE - hlen), length + hlen);
    lastOutActivity = millis();
    return (rc == hlen + length);
}

void MQTT::reconnect(String id)
{
    reconnect(id, "", "");
}

void MQTT::reconnect(String id, String user, String pass)
{
    while (!this->connected())
    {
        Serial.println("Attempting MQTT connection...");

        if (this->connect(id, user, pass))
        {
            Serial.println("MQTT connect success");
        }
        else
        {
            Serial.println("MQTT connect failed, RC=" + String(this->state()));
            delay(5000);
        }
    }
}

void MQTT::disconnect()
{
    this->buffer[0] = MQTTDISCONNECT;
    this->buffer[1] = 0;
    _client->write(this->buffer, 2);
    _state = MQTT_DISCONNECTED;
    _client->flush();
    _client->stop();
    lastInActivity = lastOutActivity = millis();
}

uint16_t MQTT::writeString(const char *string, uint8_t *buf, uint16_t pos)
{
    const char *idp = string;
    uint16_t i = 0;
    pos += 2;
    while (*idp)
    {
        buf[pos++] = *idp++;
        i++;
    }
    buf[pos - i - 2] = (i >> 8);
    buf[pos - i - 1] = (i & 0xFF);
    return pos;
}

boolean MQTT::connected()
{
    boolean rc;
    if (_client == NULL)
    {
        rc = false;
    }
    else
    {
        rc = (int)_client->connected();
        if (!rc)
        {
            if (this->_state == MQTT_CONNECTED)
            {
                this->_state = MQTT_CONNECTION_LOST;
                _client->flush();
                _client->stop();
            }
        }
        else
        {
            return this->_state == MQTT_CONNECTED;
        }
    }
    return rc;
}

MQTT &MQTT::setServer(IPAddress ip, uint16_t port)
{
    this->ip = ip;
    this->port = port;
    this->domain = "";
    return *this;
}

MQTT &MQTT::setServer(String domain, uint16_t port)
{
    this->domain = domain;
    this->port = port;
    return *this;
}

MQTT &MQTT::setClient(Client &client)
{
    this->_client = &client;
    return *this;
}

MQTT &MQTT::setStream(Stream &stream)
{
    this->stream = &stream;
    return *this;
}

int MQTT::state()
{
    return this->_state;
}

boolean MQTT::setBufferSize(uint16_t size)
{
    if (size == 0)
    {
        // Cannot set it back to 0
        return false;
    }
    if (this->bufferSize == 0)
    {
        this->buffer = (uint8_t *)malloc(size);
    }
    else
    {
        uint8_t *newBuffer = (uint8_t *)realloc(this->buffer, size);
        if (newBuffer != NULL)
        {
            this->buffer = newBuffer;
        }
        else
        {
            return false;
        }
    }
    this->bufferSize = size;
    return (this->buffer != NULL);
}

uint16_t MQTT::getBufferSize()
{
    return this->bufferSize;
}

MQTT &MQTT::setKeepAlive(uint16_t keepAlive)
{
    this->keepAlive = keepAlive;
    return *this;
}

MQTT &MQTT::setSocketTimeout(uint16_t timeout)
{
    this->socketTimeout = timeout;
    return *this;
}