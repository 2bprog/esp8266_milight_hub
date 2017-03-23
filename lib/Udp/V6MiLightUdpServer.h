// This protocol is documented here:
// http://www.limitlessled.com/dev/

#include <Arduino.h>
#include <MiLightClient.h>
#include <WiFiUdp.h>
#include <MiLightUdpServer.h>
#include <Vector.h>

#define V6_COMMAND_LEN 8
#define V6_MAX_SESSIONS 10

#ifndef _V6_MILIGHT_UDP_SERVER
#define _V6_MILIGHT_UDP_SERVER 

enum V2CommandIds {
  V2_COLOR = 0x01,
  V2_SATURATION = 0x02,
  V2_BRIGHTNESS = 0x03,
  V2_STATUS = 0x04,
  V2_KELVIN = 0x05
};

struct V6Session {
  V6Session(IPAddress ipAddr, uint16_t port, uint16_t sessionId)
    : ipAddr(ipAddr),
      port(port),
      sessionId(sessionId),
      next(NULL)
  { }
  
  IPAddress ipAddr;
  uint16_t port;
  uint16_t sessionId;
  V6Session* next;
};

class V6MiLightUdpServer : public MiLightUdpServer {
public:
  V6MiLightUdpServer(MiLightClient*& client, uint16_t port, uint16_t deviceId)
    : MiLightUdpServer(client, port, deviceId),
      sessionId(0),
      numSessions(0),
      firstSession(NULL)
  { }
  
  ~V6MiLightUdpServer();
  
  // Should return size of the response packet
  virtual void handlePacket(uint8_t* packet, size_t packetSize);
  
  template <typename T>
  static T readInt(uint8_t* packet);
  
  template <typename T>
  static uint8_t* writeInt(const T& value, uint8_t* packet);
    
protected:
  static uint8_t START_SESSION_COMMAND[];
  static uint8_t START_SESSION_RESPONSE[];
  static uint8_t COMMAND_HEADER[];
  static uint8_t COMMAND_RESPONSE[];
  static uint8_t SEARCH_COMMAND[];
  static uint8_t LOCAL_SEARCH_COMMAND[];
  
  V6Session* firstSession;
  size_t numSessions;
  uint16_t sessionId;
  
  uint16_t beginSession();
  void sendResponse(uint16_t sessionId, uint8_t* responseBuffer, size_t responseSize);
  
  void handleStartSession();
  void handleCommand(
    uint16_t sessionId,
    uint8_t sequenceNum,
    uint8_t* cmd,
    uint8_t group,
    uint8_t checksum
  );
  
  bool handleV1BulbCommand(
    uint8_t group,
    uint32_t cmd,
    uint32_t cmdArg
  );
  
  bool handleV2BulbCommand(
    uint8_t group,
    uint32_t cmd,
    uint32_t cmdArg
  );
};

#endif
