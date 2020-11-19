#include "Particle.h"
#include <ArduinoJson.h>

class SIM7600
{
public:
    static SIM7600 *getInstance();
    static void deleteInstance();

    void initSim();

    void readResponse(String command);
    bool checkResponse(String command, String response);
    int waitForResponse(String command);

    void publishData(String data, String path);
    void subData();

    bool checkIfPinRequired();

    void checkIO();
    void readMqttMessage();
    void handleMqttMessage(String topic, String payload);

    void postDweet(String latitude, String longitude);
    void readDweet();
    void readJson();

    Vector<String> getCords();
    bool getMqttStatus();

private:
    static SIM7600 *instance;
    SIM7600();
    ~SIM7600();
    bool incomingMqttMessage;
    int countLinefeed;
    String messagePath = "";
    String messagePayload = "";
};