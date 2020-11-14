#include "Particle.h"

class SIM7600
{
public:
    static SIM7600 *getInstance();
    static void deleteInstance();
    bool sendAndReadResponse(String command);
    void initSim();
    void publishData(String data, String path);
    void checkInput();

    Vector<String> getCords();

    void postDweet(String latitude, String longitude);
    void readDweet();

private:
    static SIM7600 *instance;
    int extraWaitInMillisecondsForResponse;
    String deviceId;
    SIM7600();
    ~SIM7600();
};