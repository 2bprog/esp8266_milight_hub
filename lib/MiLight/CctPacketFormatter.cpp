#include <CctPacketFormatter.h>
#include <MiLightButtons.h>

void CctPacketFormatter::reset() {
  size_t packetPtr = 0;
  
  packet[packetPtr++] = CCT;
  packet[packetPtr++] = deviceId >> 8;
  packet[packetPtr++] = deviceId & 0xFF;
  packet[packetPtr++] = groupId;
  packet[packetPtr++] = 0;
  packet[packetPtr++] = sequenceNum;
  packet[packetPtr++] = sequenceNum++;
}
  
void CctPacketFormatter::command(uint8_t command, uint8_t arg) {
  packet[CCT_COMMAND_INDEX] = command;
}

void CctPacketFormatter::updateStatus(MiLightStatus status, uint8_t groupId) {
  command(getCctStatusButton(groupId, status), 0);
}
  
void CctPacketFormatter::increaseTemperature() {
  command(CCT_TEMPERATURE_UP, 0);
}

void CctPacketFormatter::decreaseTemperature() {
  command(CCT_TEMPERATURE_DOWN, 0);
}

void CctPacketFormatter::increaseBrightness() {
  command(CCT_BRIGHTNESS_UP, 0);
}

void CctPacketFormatter::decreaseBrightness() {
  command(CCT_BRIGHTNESS_DOWN, 0);
}

uint8_t CctPacketFormatter::getCctStatusButton(uint8_t groupId, MiLightStatus status) {
  uint8_t button = 0;
  
  if (status == ON) {
    switch(groupId) {
      case 1:
        button = CCT_GROUP_1_ON;
        break;
      case 2:
        button = CCT_GROUP_2_ON;
        break;
      case 3:
        button = CCT_GROUP_3_ON;
        break;
      case 4:
        button = CCT_GROUP_4_ON;
        break;
    }
  } else {
    switch(groupId) {
      case 1:
        button = CCT_GROUP_1_OFF;
        break;
      case 2:
        button = CCT_GROUP_2_OFF;
        break;
      case 3:
        button = CCT_GROUP_3_OFF;
        break;
      case 4:
        button = CCT_GROUP_4_OFF;
        break;
    }
  }
  
  return button;
}