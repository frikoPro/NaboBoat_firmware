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

SIM7600::SIM7600() : extraWaitInMillisecondsForResponse(200), deviceId(System.deviceID()) {}

bool SIM7600::sendAndReadResponse(String command)
{
    // Send the command
    Serial.print("Sending: ");
    Serial.println(command);
    Serial1.println(command);
    int requestStartTime = millis();
    int millisecondsSinceRequestStarted = 0;
    bool wegotResponse = false;

    String errorCheck = "\nERROR\n";
    String errorTemp = "";
    bool errorReponse = true;
    int count = 0;

    // Setup a timeout
    requestStartTime = millis();

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

    // Print out the results
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
        delay(extraWaitInMillisecondsForResponse);

        // Print out the response to Serial monitor
        while (Serial1.available())
        {
            if (errorTemp.compareTo(errorCheck))
            {
                errorReponse = false;
            }
            char ch = Serial1.read();
            if (ch == errorCheck.charAt(count))
            {
                errorTemp += ch;
                count++;
            }
            else
            {
                errorTemp = "";
                count = 0;
            }
            if (ch)
            {
                Serial.print(ch);
            }
        }
        Serial.println("\n>");
    }

    return errorReponse;
}

void SIM7600::initSim()
{
    sendAndReadResponse("AT+IPR=19200");
    sendAndReadResponse("AT");
    sendAndReadResponse("AT+CPIN=" + pinCode);
    sendAndReadResponse("AT+CFUN=1");
    sendAndReadResponse("AT+CGACT=1,1");
    sendAndReadResponse("AT+CGDCONT=1,\"IP\",\"telenor.smart\"");
    sendAndReadResponse("AT+CGSOCKCONT=1,\"IP\",\"telenor.smart\"");
    sendAndReadResponse("AT+CSOCKSETPN=1");
    sendAndReadResponse("AT+CGPS=1");
    sendAndReadResponse("AT+CGREG?");
    sendAndReadResponse("AT+NETOPEN");
    sendAndReadResponse("AT+IPADDR");
}

void SIM7600::publishData(String data, String path)
{
    sendAndReadResponse("AT+CMQTTSTART");
    //clientId må være max 23 tegn
    sendAndReadResponse("AT+CMQTTACCQ=0, \"" + System.deviceID().remove(deviceId.length() - 2) + "\"");
    // må være tcp protokoll som angitt i manualen
    sendAndReadResponse("AT+CMQTTCONNECT=0,\"tcp://data.jensa.no:1883\",90,1, \"" + mqttUsername + "\"" + ",\"" + *mqttPassword + "\"");
    String topicPath = "naboBåtData/" + deviceId + "/" + path;
    char topicCommand[50];
    char payloadCommand[50];
    // max størrelsen avgjør avsluttende melding
    sprintf(payloadCommand, "AT+CMQTTPAYLOAD=0,%d", data.length());
    sprintf(topicCommand, "AT+CMQTTTOPIC=0,%d", topicPath.length());

    // topic command med max størrelse
    sendAndReadResponse(topicCommand);
    // print topic path
    Serial1.print(topicPath);
    delay(20);
    // angi payload command med max størrelse
    sendAndReadResponse(payloadCommand);
    // print data
    Serial1.print(data);
    delay(20);
    // send data
    sendAndReadResponse("AT+CMQTTPUB=0,1,60");
}

void SIM7600::checkInput()
{
    if (Serial.available() > 0)
    {
        Serial.print(">");
        delay(100);
        while (Serial.available())
        {
            char ch = Serial.read();
            Serial.print(ch);
            Serial1.print(ch);
        }
    }
    if (Serial1.available() > 0)
    {
        while (Serial1.available())
        {
            char ch = Serial1.read();
            if (ch)
            {
                Serial.print(ch);
            }
        }
    }
}

Vector<String> SIM7600::getCords()
{

    Serial1.println("AT+CGPSINFO");
    delay(20);

    int count = 0;
    Vector<String> cords;
    String longitude = "";
    String latitude = "";

    while (Serial1.available() > 0)
    {

        char ch = Serial1.read();
        if (ch)
        {
            if (ch != '.')
            {
                if (count > 24 && count < 36)
                {
                    if (ch == ',' && count == 25)
                        return cords;
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

    cords.append(latitude);
    cords.append(longitude);

    Serial.print("latitude: ");
    Serial.println(cords.first());
    Serial.print("longitude: ");
    Serial.println(cords.last());

    return cords;
}

void SIM7600::postDweet(String latitude, String longitude)
{

    String getRequest = "GET http://www.dweet.io/dweet/for/" + System.deviceID() + "?latitude=" + latitude + "&longitude=" + longitude + " HTTP/1.1\n";
    Serial.println(getRequest);
    sendAndReadResponse("AT+CHTTPACT=\"dweet.io\",80");
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
    String request = "GET /get/latest/dweet/for/2a003b000a47373336323230 HTTP/1.1\r\nHost: www.dweet.io\r\n\r\n";
    sendAndReadResponse("AT+CHTTPSSTART");
    sendAndReadResponse("AT+CHTTPSOPSE=\"dweet.io\", 80, 1");
    sendAndReadResponse("AT+CHTTPSSEND=" + String(request.length()));
    sendAndReadResponse(request);
    delay(1000);
    sendAndReadResponse("AT+CHTTPSRECV=4000");
    checkInput();
    sendAndReadResponse("AT+CHTTPSCLOSE");
    sendAndReadResponse("CHTTPSSTOP");
}
