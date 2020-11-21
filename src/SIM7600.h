#ifndef SIM7600_H
#define SIM7600_H
#include "Particle.h"
#include <ArduinoJson.h>
#include "Boat.h"

class SIM7600
{
public:
    static SIM7600 *getInstance();
    static void deleteInstance();

    void initSim();

    void readResponse(String command);
    bool checkResponse(String command, String response);
    int waitForResponse(String command);

    void publishData(String data);
    void subData();

    bool checkIfPinRequired();

    void readMqttMessage();
    void handleMqttMessage(String payload);

    void postDweet(String latitude, String longitude);
    void readDweet();
    void readJson();

    void getCords();
    bool getMqttStatus();
    void setMqttStatus(bool status);

private:
    static SIM7600 *instance;
    SIM7600();
    ~SIM7600();
    bool incomingMqttMessage;
    int countLinefeed;
    String messagePayload = "";
};
#endif