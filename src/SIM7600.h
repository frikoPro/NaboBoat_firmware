#ifndef SIM7600_H
#define SIM7600_H
#include "Particle.h"
#include <ArduinoJson.h>
#include "Boat.h"
#include <ParticleSoftSerial.h>

class SIM7600
{
public:
    static SIM7600 *getInstance();
    static void deleteInstance();

    void initSim();
    void connectMqtt();

    void readResponse(String command);
    bool checkResponse(String command, String response);
    int waitForResponse(String command);

    void publishData(String data, String path);
    void subData();
    void setLastWill();

    void readMqttMessage();
    void handleMqttMessage(String payload);

    void readDweet();
    void readJson();

    void getCords();
    bool getMqttStatus();
    void setMqttStatus(bool status);

private:
    static SIM7600 *instance;
    SIM7600();
    ~SIM7600();

    // mqtt message might arrive partially from multiple loops
    // keep them global and reset them after linefeed reaches more than 4
    bool incomingMqttMessage;
};
#endif