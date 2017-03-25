#include <MiLightClient.h>
#include <MiLightRadioConfig.h>
#include <Arduino.h>

MiLightRadio* MiLightClient::switchRadio(const MiLightRadioType type) {
  RadioStack* stack = NULL;
  
  for (int i = 0; i < numRadios; i++) {
    if (radios[i]->config.type == type) {
      stack = radios[i];
      break;
    }
  }
  
  if (stack != NULL) {
    MiLightRadio *radio = stack->getRadio();
    
    if (currentRadio->config.type != stack->config.type) {
      radio->configure();
    }
    
    currentRadio = stack;
    formatter = stack->config.packetFormatter;
    return radio;
  } else {
    Serial.print("MiLightClient - tried to get radio for unknown type: ");
    Serial.println(type);
  }
  
  return NULL;
}

void MiLightClient::prepare(MiLightRadioConfig& config, 
  const uint16_t deviceId, 
  const uint8_t groupId) {
  
  switchRadio(config.type);
  
  if (deviceId >= 0 && groupId >= 0) {
    formatter->prepare(deviceId, groupId);
  }
}

void MiLightClient::setResendCount(const unsigned int resendCount) {
  this->resendCount = resendCount;
}

bool MiLightClient::available() {
  if (currentRadio == NULL) {
    return false;
  }
  
  return currentRadio->getRadio()->available();
}

void MiLightClient::read(uint8_t packet[]) {
  if (currentRadio == NULL) {
    return;
  }
  
  size_t length;
  currentRadio->getRadio()->read(packet, length);
}

void MiLightClient::write(uint8_t packet[]) {
  if (currentRadio == NULL) {
    return;
  }
  
  for (int i = 0; i < this->resendCount; i++) {
    currentRadio->getRadio()->write(packet, currentRadio->config.getPacketLength());
  }
}
    
void MiLightClient::updateColorRaw(const uint8_t color) {
  formatter->updateColorRaw(color);
  flushPacket();
}

void MiLightClient::updateHue(const uint16_t hue) {
  formatter->updateHue(hue);
  flushPacket();
}

void MiLightClient::updateBrightness(const uint8_t brightness) {
  const MiLightRadioType type = currentRadio->config.type;
  
  if (type == CCT || type == RGB) {
    const unsigned int oldResend = resendCount;
    setResendCount(MILIGHT_DEFAULT_RESEND_COUNT);
    
    for (int i = 0; i < MILIGHT_CCT_INTERVALS; i++) {
      decreaseBrightness();
    }
    for (int i = 0; i < brightness/(100/MILIGHT_CCT_INTERVALS); i++) {
      increaseBrightness();
    }  
    
    setResendCount(oldResend);
  } else {
    formatter->updateBrightness(brightness);
    flushPacket();
  }
}
    
void MiLightClient::updateStatus(MiLightStatus status, uint8_t groupId) {
  formatter->updateStatus(status, groupId);
  flushPacket();
}

void MiLightClient::updateStatus(MiLightStatus status) {
  formatter->updateStatus(status);
  flushPacket();
}

void MiLightClient::updateSaturation(const uint8_t value) {
  formatter->updateSaturation(value);
  flushPacket();
}

void MiLightClient::updateColorWhite() {
  formatter->updateColorWhite();
  flushPacket();
}

void MiLightClient::pair() {
  for (size_t i = 0; i < 5; i++) {
    formatter->updateStatus(ON);
    flushPacket();
    delay(1);
  }
}

void MiLightClient::unpair() {
  const MiLightRadioType type = currentRadio->config.type;
  
  if (type == RGBW) {
    formatter->updateStatus(ON);
    flushPacket();
    yield();
    formatter->updateColorWhite();
    flushPacket();
  } else if (type == CCT) {
    for (int i = 0; i < 5; i++) {
      formatter->updateStatus(ON);
      flushPacket();
      delay(1);
    }
  } else if (type == RGB_CCT) {
    for (int i = 0; i < 5; i++) {
      formatter->updateStatus(ON, 0);
      flushPacket();
      delay(1);
    }
  }
}
    
void MiLightClient::increaseBrightness() {
  formatter->increaseBrightness();
  flushPacket();
}

void MiLightClient::decreaseBrightness() {
  formatter->decreaseBrightness();
  flushPacket();
}

void MiLightClient::increaseTemperature() {
  formatter->increaseTemperature();
  flushPacket();
}

void MiLightClient::decreaseTemperature() {
  formatter->decreaseTemperature();
  flushPacket();
}

void MiLightClient::updateTemperature(const uint8_t temperature) {
  MiLightRadioType type = currentRadio->config.type;
  
  if (type == CCT) {
    const unsigned int oldResend = resendCount;
    setResendCount(MILIGHT_DEFAULT_RESEND_COUNT);
    
    for (int i = 0; i < MILIGHT_CCT_INTERVALS; i++) {
      decreaseTemperature();
    }
    for (int i = 0; i < temperature/(100/MILIGHT_CCT_INTERVALS); i++) {
      increaseTemperature();
    }  
    
    setResendCount(oldResend);
  } else {
    formatter->updateTemperature(temperature);
    flushPacket();
  }
}

void MiLightClient::command(uint8_t command, uint8_t arg) {
  formatter->command(command, arg);
  flushPacket();
}

void MiLightClient::formatPacket(uint8_t* packet, char* buffer) {
  formatter->format(packet, buffer);
}
    
void MiLightClient::flushPacket() {
  PacketStream& stream = formatter->buildPackets();
  
  while (stream.hasNext()) {
    write(stream.next());
    
    if (stream.hasNext()) {
      delay(10);
    }
  }
  
  formatter->reset();
}
