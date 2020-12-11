#include "SIM7600.h"
#include "Creditentials.h"

SIM7600 *SIM7600::instance = nullptr;

// bluetooth coms, must be another variable name
ParticleSoftSerial bt(D2, D3);

SIM7600 *SIM7600::getInstance()
{
    if (!instance)
    {
        instance = new SIM7600();
    }
    return instance;
}

void SIM7600::deleteInstance()
{
    if (instance)
    {
        delete instance;
        instance = nullptr;
    }
}

SIM7600::SIM7600() : incomingMqttMessage(false) {}

// Wait for the whole response
int SIM7600::waitForResponse(String command)
{
    Serial1.println(command);
    int requestStartTime = millis();
    int millisecondsSinceRequestStarted = 0;
    bool wegotResponse = false;

    // Wait until we get a response (or timeout)
    while (!wegotResponse || millisecondsSinceRequestStarted < 2000)
    {
        millisecondsSinceRequestStarted = millis() - requestStartTime;
        if (Serial1.available() > 0)
        {
            wegotResponse = true;
        }
        Particle.process();
    }

    return millisecondsSinceRequestStarted;
}

// read the response
void SIM7600::readResponse(String command)
{
    int millisecondsSinceRequestStarted = waitForResponse(command);
    if (millisecondsSinceRequestStarted < 2000)
    {
        Serial.print("\nCommand: '");

        Serial.print(command);
        Serial.println(" timed out?\n");
    }

    else if (Serial1.available() > 0)
    {
        Serial.print("<\n");

        delay(200);

        while (Serial1.available())
        {

            char ch = Serial1.read();

            if (ch)
            {
                bt.print(ch);
                Serial.print(ch);
            }
        }

        Serial.println("\n>");
    }
}

// Lese responsen and compare, return boolean based on that
bool SIM7600::checkResponse(String command, String response)
{
    String cacheString = "";
    int count = 0;
    bool expectedResponse = false;

    int millisecondsSinceRequestStarted = waitForResponse(command);
    if (millisecondsSinceRequestStarted < 2000)
    {
        Serial.print("\nCommand: '");
        Serial.print(command);
        Serial.println(" timed out?\n");
    }

    else if (Serial1.available() > 0)
    {

        delay(200);

        while (Serial1.available())
        {
            char ch = Serial1.read();

            if (ch)
            {
                // Is char equal to first char in expected reply
                // if, add to the cacheString and then check next char
                if (ch == response.charAt(count))
                {
                    cacheString += ch;
                    count++;
                    // if response is equal, nothing more to do, return true
                    if (response.equals(cacheString))
                    {
                        expectedResponse = true;
                        return expectedResponse;
                    }
                }
                // char is not equal, reset comparison
                else
                {
                    cacheString = "";
                    count = 0;
                }
            }
        }
    }

    return expectedResponse;
}

// read JSON data
void SIM7600::readJson()
{

    int count = 0;
    String jsonString = "";
    StaticJsonDocument<300> doc;
    int timeOnSend = millis();
    int timePastSend = 0;

    // wait at least two seconds for whole response, else return
    while (timePastSend < 2000)
    {
        timePastSend = millis() - timeOnSend;
        if (Serial1.available() > 0)
        {
            char ch = Serial1.read();
            if (ch)
            {
                if (ch == '{')
                {
                    jsonString += ch;
                    count++;
                }
                else if (ch == '}')
                {
                    jsonString += ch;
                    count--;
                    if (count == 0)
                    {
                        break;
                    }
                }
                else if (count > 0)
                {
                    jsonString += ch;
                }
            }
        }
    }

    if (timePastSend >= 2000)
    {
        Serial.println("No response");
        return;
    }

    char json[jsonString.length()];

    sprintf(json, jsonString);

    //Convert to JSON object
    DeserializationError error = deserializeJson(doc, json);

    if (error)
    {
        Serial.print(F("deserializeJson() failed: "));
        return;
    }

    bool unlock = doc["with"][0]["content"]["unlock"];
    Boat::getInstance()->setStatus(unlock);
    // confirmed recieved data
    publishData(unlock ? "unlocked" : "locked", "confirmStatus");
}

// Initialize GSM-module, if pincode is required, its called
void SIM7600::initSim()
{
    readResponse("AT");
    readResponse("AT+CPIN=" + pinCode);
    readResponse("AT+CFUN=1");
    readResponse("AT+CGACT=1,1");
    readResponse("AT+CGDCONT=1,\"IP\",\"telenor.smart\"");
    readResponse("AT+CGPS=1");
    readResponse("AT+CGREG?");
    readResponse("AT+NETOPEN");
    readResponse("AT+IPADDR");
    connectMqtt();
    readDweet();
    Serial.println("\ninitialization successful");
}

void SIM7600::connectMqtt()
{
    String id = System.deviceID();
    readResponse("AT+CMQTTSTART");
    // Set client Id, max_length 23
    readResponse("AT+CMQTTACCQ=0, \"" + id.remove(id.length() - 2) + "\"");
    // connect string
    readResponse("AT+CMQTTCONNECT=0,\"tcp://161.35.167.71:1883\",90,1, \"" + mqttUsername + "\"" + ",\"" + mqttPassword + "\"");

    subData();
    setLastWill();
}

//Publish to mqtt topic
void SIM7600::publishData(String data, String path)
{
    String id = System.deviceID();
    String topicPath = id + "/" + path;

    char topicCommand[50];
    char payloadCommand[50];

    //Announce topicpath will come, set topic length for ending command
    sprintf(topicCommand, "AT+CMQTTTOPIC=0,%d", topicPath.length());
    // Announce payload will come, set payload length for ending command
    sprintf(payloadCommand, "AT+CMQTTPAYLOAD=0,%d", data.length());

    // if ERROR response, check if pin required, else check mqtt connection
    if (checkResponse(topicCommand, "ERROR"))
    {
        // bt.println("Publish failed");
        Serial.println("Publish failed");
        if (checkResponse("AT+CPIN?", "+CPIN: SIM PIN"))
        {
            Serial.println("\nSIM PIN required, starting initialization");
            bt.println("\nSIM PIN required, starting initialization");
            initSim();
            return;
        }
        else if (!checkResponse("AT+CMQTTCONNECT?", "tcp://161.35.167.71:1883"))
        {
            Serial.println("\nNot connected to MQTT, connecting now");
            bt.println("\nNot connected to MQTT, connecting now");
            connectMqtt();
            return;
        }
    }
    readResponse(topicPath);
    readResponse(payloadCommand);
    readResponse(data);
    readResponse("AT+CMQTTPUB=0,1,60");
}

// Subscribe to mqtt topic
void SIM7600::subData()
{
    String id = System.deviceID();

    String topicPath = id + "/#";

    char topicCommand[50];

    sprintf(topicCommand, "AT+CMQTTSUBTOPIC=0,%d,2", topicPath.length());
    readResponse(topicCommand);
    readResponse(topicPath);
    readResponse("AT+CMQTTSUB=0");
}

// If disconnection happens unintentionally, post to this topic, with this message
void SIM7600::setLastWill()
{

    String lastWillTopic = System.deviceID() + "/lastWill";
    String lastWillMsg = "true";
    char topicCommand[50];
    char msgCommand[50];
    sprintf(topicCommand, "AT+CMQTTWILLTOPIC=0, %d", lastWillTopic.length());
    sprintf(msgCommand, "AT+CMQTTWILLMSG=0, %d, 2", lastWillMsg.length());

    readResponse(topicCommand);
    readResponse(lastWillTopic);
    readResponse(msgCommand);
    readResponse(lastWillMsg);
}

// get latitude and longitude
void SIM7600::getCords()
{

    waitForResponse("AT+CGPSINFO");
    delay(20);

    int count = 0;
    String longitude = "";
    String latitude = "";

    while (Serial1.available() > 0)
    {

        char ch = Serial1.read();
        if (ch)
        {
            if (ch != '.')
            {
                // if first character of coordinates is ",", then break loop
                if (count == 25 && ch == ',')
                    break;
                if (count > 24 && count < 36)
                {
                    latitude += ch;
                    if (count == 26)
                        latitude += '.';
                }
                if (count >= 39 && count < 51)
                {
                    longitude += ch;
                    if (count == 41)
                        longitude += '.';
                }
            }
        }

        count++;
    }

    Serial1.flush();

    // if empty then don't replace
    if (latitude != "" && longitude != "")
    {
        Boat::getInstance()->setLatitude(latitude);
        Boat::getInstance()->setLongitude(longitude);
    }
}

// read json data from dweet.io
void SIM7600::readDweet()
{
    readResponse("AT+HTTPINIT");
    readResponse("AT+HTTPPARA=\"URL\", \"http://dweet.io/get/latest/dweet/for/" + System.deviceID() + "\"");
    readResponse("AT+HTTPACTION=0");
    delay(1000);
    Serial1.println("AT+HTTPREAD=0, 300\r");
    readJson();
}

// get payload from mqttMessage
void SIM7600::readMqttMessage()
{
    int countLinefeed = 0;
    String messagePayload = "";
    int timeOnRecv = millis();
    int timePastRecv = 0;
    // count linefeeds
    // payload starts at linefeed number 4
    delay(10);
    while (timePastRecv < 2000)
    {
        timePastRecv = millis() - timeOnRecv;
        if (Serial1.available() > 0)
        {
            char ch = Serial1.read();
            if (ch)
            {

                if (ch == '\n')
                {
                    countLinefeed++;
                }

                //exclude carriage return and linefeed
                else if (countLinefeed == 4 && ch != '\r' && ch != '\n')
                {
                    messagePayload += ch;
                }

                else if (countLinefeed > 4)
                {
                    handleMqttMessage(messagePayload);
                    incomingMqttMessage = false;
                    return;
                }
            }
        }
    }

    incomingMqttMessage = false;
    Serial.print("failed to recieve mqtt message");
}

// handle JSON object from payload
void SIM7600::handleMqttMessage(String payload)
{
    StaticJsonDocument<300> doc;
    Boat *boat = Boat::getInstance();
    char json[payload.length()];
    sprintf(json, payload);
    DeserializationError error = deserializeJson(doc, json);

    if (error)
    {
        Serial.print(F("deserializeJson() failed: "));
        return;
    }

    bool status = doc["unlock"];

    boat->setStatus(status);

    // // confirm message recieved
    publishData(status ? "unlocked" : "locked", "confirmStatus");
}

// return current state, if true, incoming mqtt message
bool SIM7600::getMqttStatus()
{
    return incomingMqttMessage;
}

// set if incoming mqtt message has arrived
void SIM7600::setMqttStatus(bool status)
{
    incomingMqttMessage = status;
}