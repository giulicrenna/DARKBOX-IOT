/*
apMode.h
This header contains all the neccesary methods to succesfully set the ESP8266 into
an access point, where you can configure and save the following parameters into the EEPROM
in a file called config.json
parameters:
|_ wifi ssid
|_ wifi password
|_ static ipv4
|_ gateway
|_ subnet mask
|_ device name
|_ smtp gmail (where will be sent all the alerts)

the ap mode also allow yo to upload a new .bin or .elf firmware
wich will be uploaded using OTA.

To access into this configuration you have to connect to the WiFi network
generated by the ESP8266 the network SSID is "DarkFlow_xxxxxxxx" where the x part
is the UID (Unique ID) of the device.
*/

#ifndef APMODE_H
#define APMODE_H

#ifndef AP_OUTLINE
#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#endif
#include "functions.hpp"
#ifdef I2C
#include "screenController.hpp"
#endif

IPAddress local_ip(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(8, 8, 8, 8);

void changeCredentials(fs::FS &fs, const char *path, String mailReceiver,
                       String StaticIP, String gateway, String subnet, String ssid, String password, String deviceName);
void saveConfigCallback();
String dataAsString(const char *path);

WiFiManager myManager;

#ifdef I2C
Screen myScreenAp;
#endif

bool shouldSaveConfig = false;

/**
 * @brief apMode class
 *
 */

/*
apMode is the main class is where is stored all the required data to make the library work
and where are the neccessary methods to setup all the connections.

*/

class apMode
{
private:
  const char *ssid = String("darkflow-" + chipId).c_str();
  const char *password = "123456789";
  const char *icon = "<link rel='icon' type='image/png' sizes='16x16' href='data:image/x-icon;base64,AAABAAEAEBAAAAEAIABoBAAAFgAAACgAAAAQAAAAIAAAAAEAIAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAADv7+//7+/v/+/v7//v7+//7+/v//Hw7//y8e//8fHv//Lx7//y8u//8vHv/+/v7//v7+//7+/v/+/v7//v7+//FxcX/xcXF/8XFxf/FxcX/xcXF/8QERf/DA4X/wwOF/8MDhf/CQsW/w0OF/8YGBf/GBgX/xcXF/8XFxf/FxcX/wAAAP8AAAD/AAAA/wIBAP8AAAD/XE0A/5R7AP+JcgD/iHEA/21bAP8uJQD/AAAA/wAAAP8AAAD/AAAA/wAAAP8BAQH/AQEB/wAAAf8EAwL/AAAB/5V+CP/sxwv/478L/+zGC//wygv/478L/5eAB/8YFAL/AAAB/wMDAv8BAQH/AAAA/wwKAf9iUwT/NSwC/wAAAP+KdAb/478K/1JFBP8kHgL/XE0E/66SB//sxwr/spYI/xANAf8AAAD/AQEA/wAAAP9wXgX/+dEK/0k9A/8AAAD/i3UG/+G9Cv9qWQX/NCwD/wMCAP8FBAD/kHkG/+3ICv95ZQX/AAAA/wMCAP8AAAD/c2EF/+rFCv9DOQP/AAAA/5mBBv/uyAr/6cQK//LLCv80LAL/AAAA/xkVAf/JqQn/w6QI/wsJAf8AAAD/AAAA/3JgBf/txwr/QzkD/wAAAP9SRAT/fGgF/29dBf96ZwX/LygC/wAAAP8AAAD/lHwG/967Cv8iHQL/AAAA/wAAAP9yYAX/7ccK/0M5A/8AAAD/YVIE/5R9Bv+Icwb/knoG/3FfBf8AAAD/AAAA/4dyBv/jvwn/KCEC/wAAAP8AAAD/cmAF/+3HCv9EOgP/AAAA/52EB//0zgr/478K/+rFCv/Ztwn/HBcC/wAAAP+skQf/1rQJ/xoWAf8AAAD/AAAA/3JgBf/txwr/RDkD/wAAAP8oIQL/OC8C/zUtAv81LQL/OzED/wQDAP86MQP/4L0J/62SB/8AAAD/AAAA/wAAAP9yYAX/7ccK/0A2A/8AAAD/AwIA/wAAAP8AAAD/AQEA/wAAAP8qIwL/vZ8I/+fDCv9URwT/AAAA/wMCAP8AAAD/cV8F/+XACv9wXgX/PzUD/0s/A/9KPgP/T0IE/2NTBP+QeQb/1LMJ/+rFCv+FcAb/AAAA/wEBAP8AAAD/AAAA/35qBf/yywr/58EK/+/ICv/uyAr/7sgK/+/ICv/wygr/68YK/8ioCP9lVQX/AwMA/wEAAP8AAAD/AAAA/wAAAP9BNwP/fGgF/3BeBf9xXwX/cmAF/3JfBf9yYAX/alkF/0c8A/8SDwH/AAAA/wICAP8BAAD/AAAA/wAAAP8BAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wMCAP8BAAD/AAAA/wAAAP8AAAD/AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==' />";
  String name = String("darkflow-") + chipId;
  char smtpMail[40];
  char staticIPAP[40];
  char gatewayAP[40];
  char subnetAP[40];
  char deviceName_[40];

public:
  /*
  setupServer : String staticIpAp, String gatewayAP_, String subnetMaskAp -> None (bad called params)
  This method needs Strings as parameters, this strings are the static configuration
  of the ESP when this connects as a client to an external network.

  Observation: The method setuServer() has to be executed in the setup line from the main file
  */
  void setupServer()
  {
    // Library Configuration
    WiFi.mode(WIFI_STA);

    //WiFi.setSleepMode(WIFI_NONE_SLEEP);

    //myManager.setHostname(localDeviceName);

    myManager.setCustomHeadElement(icon);

    myManager.setAPStaticIPConfig(local_ip, gateway, subnet);

    myManager.setSaveConfigCallback(saveConfigCallback);

    myManager.setDarkMode(true);

    myManager.setTitle("Darkbox");

    myManager.setTimeout(520);

    myManager.setRemoveDuplicateAPs(true);

    myManager.setCleanConnect(true);

    myManager.setMinimumSignalQuality(35);

    myManager.setScanDispPerc(true);

    //myManager.setCaptivePortalEnable(true);

    myManager.setHostname(name.c_str());

    // Shows information trough the I2C screen

    #ifdef I2C
    String msg0 = "SSID:" + String(ssid);
    myScreenAp.printScreen("CONFIGURE WIFI HDP..", 0, 0, true);
    myScreenAp.printScreen(msg0, 0, 1, false);
    myScreenAp.printScreen("IP: 192.168.1.1", 0, 2, false);
    myScreenAp.printScreen("2 Minutes timeout", 0, 3, false);
    #endif

    // Custom parameters
    /*
    WiFiManagerParameter deviceName("deviceName", "Nombre del dispositivo (Campo obligatorio)", deviceName_, 40, "required");
    myManager.addParameter(&deviceName);
    WiFiManagerParameter smtpUser("mail", "SMTP mail", smtpMail, 40);
    myManager.addParameter(&smtpUser);
    WiFiManagerParameter customWarning("<p>Keep all network parameters in blank for DHCP</p>"); // Sensor 0 Parameters
    myManager.addParameter(&customWarning);
    WiFiManagerParameter StaticIpParam("StaticIP", "IP estatica", staticIPAP, 40, "pattern='\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}'");
    myManager.addParameter(&StaticIpParam);
    WiFiManagerParameter gatewayParam("GateWay", "Puerta de enlace predeterminada", gatewayAP, 40, "pattern='\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}'");
    myManager.addParameter(&gatewayParam);
    WiFiManagerParameter subnetParam("Subnet", "Mascara de subred", subnetAP, 40, "pattern='\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}'");
    myManager.addParameter(&subnetParam);
    */

    //<-

    //-> Start the client
    myManager.autoConnect(name.c_str());

    //-> Save config into JSON

    /*
    strcpy(smtpMail, smtpUser.getValue());
    strcpy(staticIPAP, StaticIpParam.getValue());
    strcpy(gatewayAP, gatewayParam.getValue());
    strcpy(subnetAP, subnetParam.getValue());
    strcpy(deviceName_, deviceName.getValue());
    */

    if (shouldSaveConfig)
    {
      changeCredentials(LittleFS, "/config.json", String(smtpMail), String(staticIPAP),
                        String(gatewayAP), String(subnetAP), myManager.getWiFiSSID(), myManager.getWiFiPass(), String(deviceName_));
      ESP.restart();
    }

    localIP = WiFi.localIP().toString();
    Serial.println(localIP);
  }
  /*
  reset: None -> None
  This function is simply used to reset all the ESP configuration
  */
  void reset()
  {
    Serial.println("*** Resetting WiFi credentials ***");
    delay(5000);
    myManager.resetSettings();
    ESP.eraseConfig();
    ESP.reset();
    ESP.restart();
  }
  //<-
};

/*
saveConfigCallback: None -> None
This function is used by the library WiFimanager.h as a flag to save the parameters at the
end of the configuration instance of the library.
In conclution this function is used to call only one time another funtion to do wherever
the programmer wants. (In this case changeCredentials will be called)
*/
void saveConfigCallback()
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

/*
changeCredentials: fs::FS fs (File system), const char *path (path where the data will be stored), String mailReceiver,
                    String StaticcIP, String gateway, String  subnet, String ssid, String password, String deviceName -> None
this function receive all the neccesary data to write in the EEPROM in a JSON file.
LittleFs is used to create the file system and all the data is stored into a temporal
instance of StaticJsonDocument. This is a class fromr ArduinoJson.h
*/
void changeCredentials(fs::FS &fs, const char *path, String mailReceiver,
                       String StaticIP, String gateway, String subnet, String ssid, String password, String deviceName)
{
  #ifdef PREFERENCES
  myPref.begin("EPM", false);
  myPref.putString("deviceName", deviceName);
  myPref.putString("ssid", ssid);
  myPref.putString("wifiPassword", password);
  myPref.putString("staticIpAP", StaticIP);
  myPref.putString("subnetMaskAP", subnet);
  myPref.putString("gatewayAP", gateway);
  myPref.putString("SmtpReceiver", mailReceiver);
  myPref.end();
  #endif
  #ifndef PREFERENCES
  fs.begin();
  File file_ = fs.open(path, "w");
  String content;
  if (!file_.available())
  {
    Serial.println("Couldn't open the file (AP INSTANCE)");
  }
  StaticJsonDocument<1024> config;

  config["device"]["UID"] = chipId;
  config["device"]["name"] = deviceName.c_str();
  config["network"]["SSID"] = ssid.c_str();
  config["network"]["wifiPassword"] = password.c_str();
  config["network"]["ip"] = StaticIP.c_str();
  config["network"]["subnetMask"] = subnet.c_str();
  config["network"]["gateway"] = gateway.c_str();
  config["smtp"]["mailSender"] = "giulicrenna@outlook.com";
  config["smtp"]["mailPassword"] = "kirchhoff2002";
  config["smtp"]["mailReceiver"] = mailReceiver.c_str();
  config["smtp"]["smtpServer"] = "smtp.office365.com";
  config["smtp"]["smtpPort"] = "587";
  config["ports"]["IO_0"] = "OTD";
  config["ports"]["IO_1"] = "OTD";
  config["ports"]["IO_2"] = "OTD";
  config["ports"]["IO_3"] = "OTD";
  config["names"]["DHTSensor_hum_name"] = "humedad";
  config["names"]["DHTSensor_temp_name"] = "temperatura";
  config["names"]["TempSensor_name"] = "temperatura";
  config["names"]["d0_name"] = "digital0";
  config["names"]["d1_name"] = "digital1";
  config["names"]["d2_name"] = "digital2";
  config["names"]["d3_name"] = "digital3";
  config["etc"]["DHT"] = "50000";
  config["etc"]["SingleTemp"] = "50000";
  config["etc"]["keepAlive"] = "60000";

  Serial.println("#### CONFIG WRITTEN ####");

  auto error = serializeJsonPretty(config, file_);

  if (error)
  {
    Serial.println("Failed to Serialize (AP INSTANCE)");
  }

  file_.close();

  fs.end();
  #endif
}

int DHCPtoStatic(String staticIpAP, String gatewayAP_, String subnetMaskAP)
{
  if (staticIpAP != "" && subnetMaskAP != "" && gatewayAP_ != "")
  {
    IPAddress miIp = strToIp(staticIpAP.c_str());
    IPAddress miGateway = strToIp(gatewayAP_.c_str());
    IPAddress miSubnet = strToIp(subnetMaskAP.c_str());

    Serial.println("Local IP change to " + staticIpAP);
    myManager.setSTAStaticIPConfig(miIp, miGateway, miSubnet, IPAddress(8, 8, 8, 8)); // Repair this, static ip detected but no configured
    WiFi.config(miIp, miGateway, miSubnet);
  }

  return 0;
}
#endif