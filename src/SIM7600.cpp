#include "SIM7600.h"
#include "Creditentials.h"

SIM7600 *SIM7600::instance = nullptr;

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

SIM7600::SIM7600() : incomingMqttMessage(false), countLinefeed(0) {}

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

        // wait a little longer so we get the whole response
        delay(200);
        // Print out the response to Serial monitor
        while (Serial1.available())
        {

            char ch = Serial1.read();

            if (ch)
            {
                Serial.print(ch);
            }
        }

        Serial.println("\n>");
    }
}

bool SIM7600::checkResponse(String command, String response)
{
    String responseCache = "";
    int count = 0;
    bool correctResponse = false;

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

        // wait a little longer so we get the whole response
        delay(200);

        // Print out the response to Serial monitor
        while (Serial1.available())
        {
            char ch = Serial1.read();

            if (ch)
            {
                Serial.print(ch);
                if (ch == response.charAt(count))
                {
                    responseCache += ch;
                    count++;
                    correctResponse = true;
                    Serial1.flush();
                    return correctResponse;
                }
                else
                {
                    responseCache = "";
                    count = 0;
                }
            }
        }

        Serial.println("\n>");
    }

    return correctResponse;
}

void SIM7600::readJson()
{

    int count = 0;
    String jsonString = "";
    StaticJsonDocument<300> doc;
    int timeOnSend = millis();
    int timePastSend = 0;

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

    char json[jsonString.length()];

    sprintf(json, jsonString);

    DeserializationError error = deserializeJson(doc, json);

    if (error)
    {
        Serial.print(F("deserializeJson() failed: "));
        return;
    }

    bool unlock = doc["with"][0]["content"]["unlock"];
    Boat::getInstance()->setStatus(unlock);
    publishData(unlock ? "unlocked" : "locked", "confirmStatus");
}

void SIM7600::initSim()
{
    readResponse("AT");
    readResponse("AT+CPIN=" + pinCode);
    readResponse("AT+CFUN=1");
    readResponse("AT+CGACT=1,1");
    readResponse("AT+CGDCONT=1,\"IP\",\"telenor.smart\"");
    // sendAndReadResponse("AT+CGSOCKCONT=1,\"IP\",\"telenor.smart\"");
    // sendAndReadResponse("AT+CSOCKSETPN=1");
    readResponse("AT+CGPS=1");
    readResponse("AT+CGREG?");
    readResponse("AT+NETOPEN");
    readResponse("AT+IPADDR");
    //clientId må være max 23 tegn
    connectMqtt();
    Serial.println("\ninitialization successful");
}

void SIM7600::connectMqtt()
{
    String id = System.deviceID();
    readResponse("AT+CMQTTSTART");
    readResponse("AT+CMQTTACCQ=0, \"" + id.remove(id.length() - 2) + "\"");
    readResponse("AT+CMQTTCONNECT=0,\"tcp://161.35.167.71:1883\",90,1, \"" + mqttUsername + "\"" + ",\"" + mqttPassword + "\"");

    subData();
    setLastWill();
}

void SIM7600::publishData(String data, String path)
{
    String id = System.deviceID();
    String topicPath = id + "/" + path;
    char topicCommand[50];
    char payloadCommand[50];
    // max størrelsen avgjør avsluttende melding
    sprintf(payloadCommand, "AT+CMQTTPAYLOAD=0,%d", data.length());
    sprintf(topicCommand, "AT+CMQTTTOPIC=0,%d", topicPath.length());

    // topic command med max størrelse
    readResponse(topicCommand);
    // print topic path
    // checkResponse(topicPath, "OK");
    readResponse(topicPath);
    // angi payload command med max størrelse
    readResponse(payloadCommand);
    // print data
    // checkResponse(data, "OK");
    readResponse(data);
    // send data
    readResponse("AT+CMQTTPUB=0,1,60");
}

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

bool SIM7600::checkIfPinRequired()
{

    String cpinRequiredString = "+CPIN: SIM PIN";
    String replyString = "";
    bool pinRequired = false;
    int count = 0;
    Serial1.println("AT+CPIN?");
    delay(10);

    if (Serial1.available() > 0)
    {
        delay(20);
        while (Serial1.available())
        {
            char ch = Serial1.read();
            if (ch)
            {
                if (ch == cpinRequiredString.charAt(count))
                {
                    replyString += ch;
                    count++;
                    if (cpinRequiredString.compareTo(replyString) == 0)
                    {
                        pinRequired = true;
                    }
                }
                else
                {
                    replyString = "";
                    count = 0;
                }
            }
        }
    }

    return pinRequired;
}

bool SIM7600::checkMqttComs()
{
    String unconnectedReply = "+CMQTTCONNECT: 0\n";
    String replyString = "";
    bool reconnect = false;
    int count = 0;
    Serial1.println("AT+CMQTTCONNECT?");
    delay(10);

    if (Serial1.available() > 0)
    {
        delay(20);
        while (Serial1.available())
        {
            char ch = Serial1.read();
            if (ch)
            {
                if (ch == unconnectedReply.charAt(count))
                {
                    replyString += ch;
                    count++;
                    if (unconnectedReply.compareTo(replyString) == 0)
                    {
                        reconnect = true;
                    }
                }
                else
                {
                    replyString = "";
                    count = 0;
                }
            }
        }
    }

    return reconnect;
}

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

    if (latitude != "" && longitude != "")
    {
        Boat::getInstance()->setLatitude(latitude);
        Boat::getInstance()->setLongitude(longitude);
    }
}

void SIM7600::postDweet(String latitude, String longitude)
{

    String getRequest = "GET http://www.dweet.io/dweet/for/" + System.deviceID() + "?latitude=" + latitude + "&longitude=" + longitude + " HTTP/1.1\n";
    Serial.println(getRequest);
    readResponse("AT+CHTTPACT=\"dweet.io\",80");
    delay(200);
    Serial1.println(getRequest);
    Serial1.println("Host: dweet.io\n");
    Serial1.println("User-Agent: mozilly\n");
    Serial1.println("Content-Length: 0\n");
    Serial1.print(char(26));
    delay(20);
    Serial1.print(char(26));
}

void SIM7600::readDweet()
{
    readResponse("AT+HTTPINIT");
    readResponse("AT+HTTPPARA=\"URL\", \"http://dweet.io/get/latest/dweet/for/" + System.deviceID() + "\"");
    readResponse("AT+HTTPACTION=0");
    Serial1.println("AT+HTTPREAD=0, 300\r");
    readJson();
}

void SIM7600::readMqttMessage()
{

    delay(10);
    while (Serial1.available())
    {
        char ch = Serial1.read();
        if (ch)
        {
            if (ch == '\n')
            {
                countLinefeed++;
            }

            else if (countLinefeed == 4 && ch != '\r' && ch != '\n')
            {
                messagePayload += ch;
            }

            else if (countLinefeed > 4)
            {
                handleMqttMessage(messagePayload);
                countLinefeed = 0;
                messagePayload = "";
                incomingMqttMessage = false;
                return;
            }
        }
    }
}

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

    publishData(status ? "unlocked" : "locked", "confirmStatus");
}

bool SIM7600::getMqttStatus()
{
    return incomingMqttMessage;
}

void SIM7600::setMqttStatus(bool status)
{
    incomingMqttMessage = status;
}