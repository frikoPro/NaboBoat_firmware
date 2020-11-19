#ifndef BOAT_H
#define BOAT_H
#include "Particle.h"

class Boat
{

public:
    static Boat *getInstance();
    static void deleteInstance();
    void setStatus(bool status);
    bool getStatus();

    String getLongitude();
    void setLongitude(String newVal);

    String getLatitude();
    void setLatitude(String newVal);

private:
    static Boat *instance;
    Boat();
    ~Boat();
    bool isUnlocked;
    String latitude;
    String longitude;
};
#endif