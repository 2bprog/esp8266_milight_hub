#include <Arduino.h>
#include <MiLightRadio.h>
#include <PL1167_nRF24.h>
#include <RF24.h>
#include <MiLightButtons.h>

#ifndef _MILIGHTCLIENT_H
#define _MILIGHTCLIENT_H

#define MILIGHT_PACKET_LENGTH 7

enum MiLightRadioType {
  RGBW  = 0xB8,
  CCT   = 0x5A
};

enum MiLightStatus { ON = 0, OFF = 1 };
  
struct MiLightPacket {
  uint8_t deviceType;
  uint16_t deviceId;
  uint8_t b1;
  uint8_t b2;
  uint8_t b3;
  uint8_t sequenceNum;
};

class MiLightRadioStack {
public:
  MiLightRadioStack(RF24& rf, const MiLightRadioConfig& config) {
    nrf = new PL1167_nRF24(rf);
    radio = new MiLightRadio(*nrf, config);
  }
  
  ~MiLightRadioStack() {
    delete radio;
    delete nrf;
  }
  
  inline MiLightRadio* getRadio() {
    return this->radio;
  }
  
private:
  PL1167_nRF24 *nrf;
  MiLightRadio *radio;
};

class MiLightClient {
  public:
    MiLightClient(uint8_t cePin, uint8_t csnPin)
    : sequenceNum(0),
      rf(RF24(cePin, csnPin))
    {
      rgbwRadio = new MiLightRadioStack(rf, MilightRgbwConfig);
      cctRadio = new MiLightRadioStack(rf, MilightCctConfig);
    }
    
    ~MiLightClient() {
      delete rgbwRadio;
      delete cctRadio;
    }
    
    void begin() {
      rgbwRadio->getRadio()->begin();
      cctRadio->getRadio()->begin();
    }
    
    bool available(const MiLightRadioType radioType);
    void read(const MiLightRadioType radioType, MiLightPacket& packet);
    void write(const MiLightRadioType radioType, MiLightPacket& packet, const unsigned int resendCount = 50);
    
    void writeRgbw(
      const uint16_t deviceId,
      const uint8_t color,
      const uint8_t brightness,
      const uint8_t groupId,
      const uint8_t button
    );
    
    // Common methods
    void updateStatus(const uint16_t deviceId, const uint8_t groupId, MiLightStatus status);
    void pair(const uint16_t deviceId, const uint8_t groupId);
    void unpair(const uint16_t deviceId, const uint8_t groupId);
    void allOn(const uint16_t deviceId);
    void allOff(const uint16_t deviceId);
    void pressButton(const uint16_t deviceId, const uint8_t groupId, uint8_t button);
    
    // RGBW methods
    void updateHue(const uint16_t deviceId, const uint8_t groupId, const uint16_t hue);
    void updateBrightness(const uint16_t deviceId, const uint8_t groupId, const uint8_t brightness);
    void updateColorWhite(const uint16_t deviceId, const uint8_t groupId);
    void updateColorRaw(const uint16_t deviceId, const uint8_t groupId, const uint16_t color);

    // CCT methods
    void updateColorTemperature(const uint8_t colorTemperature);
    
    MiLightRadio* getRadio(const MiLightRadioType type);
    
    static void deserializePacket(const uint8_t rawPacket[], MiLightPacket& packet);
    static void serializePacket(uint8_t rawPacket[], const MiLightPacket& packet);
    
  private:
    RF24 rf;
    MiLightRadioStack* rgbwRadio;
    MiLightRadioStack* cctRadio;
    
    uint8_t sequenceNum;
    uint8_t nextSequenceNum();
};

#endif