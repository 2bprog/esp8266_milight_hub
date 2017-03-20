#include <RgbCctPacketFormatter.h>

#define V2_OFFSET(byte, key, jumpStart) ( \
  V2_OFFSETS[byte-1][key%4] \
    + \
  ((jumpStart > 0 && key >= jumpStart && key <= jumpStart+0x80) ? 0x80 : 0) \
)

uint8_t const RgbCctPacketFormatter::V2_OFFSETS[][4] = {
  { 0x45, 0x1F, 0x14, 0x5C },
  { 0xAB, 0x49, 0x63, 0x91 },
  { 0x2D, 0x1F, 0x4A, 0xEB },
  { 0xAF, 0x03, 0x1D, 0xF3 },
  { 0x5A, 0x22, 0x30, 0x11 },
  { 0x04, 0xD8, 0x71, 0x42 },
  { 0xAF, 0x04, 0xDD, 0x07 },
  { 0xE1, 0x93, 0xB8, 0xE4 }
};
  
uint8_t const RgbCctPacketFormatter::BYTE_JUMP_STARTS[] = {
  0, // key byte doesn't get shifted
  0x54, 0x54, 0x14, 0x54, 
  0, // argument byte gets different shifts for different commands
  0x54, 0x54,
  0  // checksum isn't shifted
};

uint8_t const RgbCctPacketFormatter::ARG_JUMP_STARTS[] = {
  0,    // no command with id = 0
  0x14, // on
  0x14, // color
  0x14, // kelvin
  0x54, // brightness, saturation
  0x14  // mode
};

void RgbCctPacketFormatter::reset() {
  size_t packetPtr = 0;
  
  packet[packetPtr++] = 0x00;
  packet[packetPtr++] = 0x20;
  packet[packetPtr++] = deviceId >> 8;
  packet[packetPtr++] = deviceId & 0xFF;
  packet[packetPtr++] = 0;
  packet[packetPtr++] = 0;
  packet[packetPtr++] = sequenceNum++;
  packet[packetPtr++] = groupId;
  packet[packetPtr++] = 0;
}
  
void RgbCctPacketFormatter::command(uint8_t command, uint8_t arg) {
  packet[RGB_CCT_COMMAND_INDEX] = command;
  packet[RGB_CCT_ARGUMENT_INDEX] = arg;
}

void RgbCctPacketFormatter::updateStatus(MiLightStatus status, uint8_t groupId) {
  command(RGB_CCT_ON, 0xC0 + groupId + (status == OFF ? 5 : 0));
}

void RgbCctPacketFormatter::updateBrightness(uint8_t brightness) {
  command(RGB_CCT_BRIGHTNESS, 0x4F + brightness);
}
  
void RgbCctPacketFormatter::updateHue(uint16_t value) {
  const int16_t remappedColor = (value + 20) % 360;
  updateColorRaw(rescale(remappedColor, 255, 360));
}

void RgbCctPacketFormatter::updateColorRaw(uint8_t value) {
  command(RGB_CCT_COLOR, 0x15 + value);
}
  
void RgbCctPacketFormatter::updateTemperature(uint8_t value) {
  command(RGB_CCT_KELVIN, (0x4C + value)*2);
}

void RgbCctPacketFormatter::updateSaturation(uint8_t value) {
  command(RGB_CCT_SATURATION, value - 0x33);
}
  
void RgbCctPacketFormatter::updateColorWhite() {
  updateTemperature(0);
}
  
uint8_t* RgbCctPacketFormatter::buildPacket() {
  encodeV2Packet(packet);
  return packet;
}

uint8_t RgbCctPacketFormatter::xorKey(uint8_t key) {
  // Generate most significant nibble
  const uint8_t shift = (key & 0x0F) < 0x04 ? 0 : 1;
  const uint8_t x = (((key & 0xF0) >> 4) + shift + 6) % 8;
  const uint8_t msn = (((4 + x) ^ 1) & 0x0F) << 4;

  // Generate least significant nibble
  const uint8_t lsn = ((((key & 0xF) + 4)^2) & 0x0F);

  return ( msn | lsn );
}

uint8_t RgbCctPacketFormatter::decodeByte(uint8_t byte, uint8_t s1, uint8_t xorKey, uint8_t s2) {
  uint8_t value = byte - s2;
  value = value ^ xorKey;
  value = value - s1;
  
  return value;
}

uint8_t RgbCctPacketFormatter::encodeByte(uint8_t byte, uint8_t s1, uint8_t xorKey, uint8_t s2) {
  uint8_t value = (byte + s1) % 0x100;
  value = value ^ xorKey;
  value = (value + s2) % 0x100;
  
  return value;
}

void RgbCctPacketFormatter::decodeV2Packet(uint8_t *packet) {
  uint8_t key = xorKey(packet[0]);
  
  for (size_t i = 1; i <= 8; i++) {
    if (i != 5) {
      packet[i] = decodeByte(packet[i], 0, key, V2_OFFSET(i, packet[0], BYTE_JUMP_STARTS[i]));
    }
  }
      
  packet[5] = decodeByte(packet[5], 0, key, V2_OFFSET(5, packet[0], ARG_JUMP_STARTS[packet[4]]));
}

void RgbCctPacketFormatter::encodeV2Packet(uint8_t *packet) {
  uint8_t key = xorKey(packet[0]);
  uint8_t sum = key;
  uint8_t command = packet[4];
  size_t ptr = 0;
  
  for (size_t i = 1; i <= 7; i++) {
    sum += packet[i];
    
    if (i != 5) {
      printf("%d s2 = %02X\n", i, V2_OFFSET(i, packet[0], BYTE_JUMP_STARTS[i]));
      packet[i] = encodeByte(packet[i], 0, key, V2_OFFSET(i, packet[0], BYTE_JUMP_STARTS[i]));
    }
  }
  
  packet[5] = encodeByte(packet[5], 0, key, V2_OFFSET(5, packet[0], ARG_JUMP_STARTS[command]));
  packet[8] = encodeByte(sum, 2, key, V2_OFFSET(8, packet[0], 0));
}

void RgbCctPacketFormatter::format(uint8_t const* packet, char* buffer) {
  buffer += sprintf(buffer, "Raw packet: ");
  for (int i = 0; i < packetLength; i++) {
    buffer += sprintf(buffer, "%02X ", packet[i]);
  }
  
  uint8_t decodedPacket[packetLength];
  memcpy(decodedPacket, packet, packetLength);
  
  decodeV2Packet(decodedPacket);
  
  buffer += sprintf(buffer, "\n\nDecoded:\n");
  buffer += sprintf(buffer, "Key      : %02X\n", decodedPacket[0]);
  buffer += sprintf(buffer, "b1       : %02X\n", decodedPacket[1]);
  buffer += sprintf(buffer, "ID       : %02X%02X\n", decodedPacket[2], decodedPacket[3]);
  buffer += sprintf(buffer, "Command  : %02X\n", decodedPacket[4]);
  buffer += sprintf(buffer, "Argument : %02X\n", decodedPacket[5]);
  buffer += sprintf(buffer, "Sequence : %02X\n", decodedPacket[6]);
  buffer += sprintf(buffer, "Group    : %02X\n", decodedPacket[7]);
  buffer += sprintf(buffer, "Checksum : %02X", decodedPacket[8]);
}
