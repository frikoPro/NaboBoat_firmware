#include "Particle.h"

class Boat
{

public:
    static Boat *getInstance();
    static void deleteInstance();
    void setStatus(bool status);
    bool getStatus();
    void setCords(Vector<String> newCords);
    Vector<String> getCords();

private:
    static Boat *instance;
    Boat();
    ~Boat();
    bool isUnlocked;
    Vector<String> cords;
};