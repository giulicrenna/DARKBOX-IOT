#define AP_OUTLINE
#define AP_CUSTOM_PORTAL
//#define DASHBOARD_OUTLINE
#define LOCAL_DASHBOARD
//#define SMTP_CLIENT
//#define I2C 
#define PREFERENCES
#define DEBUG

#ifdef PREFERENCES
Preferences myPref;
#endif
typedef enum{
  DNS_UPDATE,
  SCREEN_REFRESH,
  TEMPORAL_DATA,
  CHECK_STATUS,
  MQTT_DHT,
  MQTT_SINGLE_TEMP,
  MQTT_KEEP_ALIVE,
  MQTT_SEND_RELAY_STATUS,
  MQTT_POLL,
  CONN_CHECK
}States;

States currentState = TEMPORAL_DATA;

unsigned int password = 123456789;
unsigned int port;
unsigned int eventInterval = 1500;
unsigned long int previousTimeScreen = 0;
unsigned long int previousTimeTemporalData = 0;
unsigned long int previousTimeTemporalCheckConnection = 0;
unsigned long int previousTimeMQTT_DHT = 0;
unsigned long int previousKeepAliveTime = 0;
unsigned long int previousMQTTsingleTemp = 0;
unsigned long int previousReleStatusSendTime = 0;
unsigned int temporalDataRefreshTime = 100;
unsigned int SmtpPort;
unsigned long long MQTTDHT;
unsigned long long MQTTsingleTemp;
unsigned long long keepAliveTime;
unsigned long long int releStatusSendTime;

String chipId = String(ESP.getChipId());
String IO_0;
String IO_1;
String IO_2;
String IO_3;
String deviceName;

String localIP;
String staticIpAP;
String subnetMaskAP;
String gatewayAP;

String mqtt_username;
String mqtt_password;

String SmtpSender;
String SmtpPass;
String SmtpReceiver;
String SmtpServer;
String heap;
String bootVersion = String(ESP.getBootVersion());
String chipId_;
String CPUfreq = String(ESP.getCpuFreqMHz());
String coreVersion = String(ESP.getCoreVersion());
String flashChipId = String(ESP.getFlashChipId());
String flashRealSize = String(ESP.getFlashChipRealSize());
String flashChipSpeed = String(ESP.getFlashChipSpeed());
String freeSketchSize = String(ESP.getFreeSketchSpace());
String fullVersion = String(ESP.getFullVersion());
String vcc = "0";
String releStatus = "LOW";
String localDeviceName = String("darkflow-") + chipId;
String tempString0, tempString1, tempString2;
String host;
String configTopic = "DeviceConfig/" + chipId;
String root_topic_subscribe = "DeviceConfig/" + chipId;
String root_topic_publish = "DeviceData/" + chipId;
String keep_alive_topic_publish = "DeviceStatus/" + chipId;
String reset_topic = "ResetDevice/" + chipId;
String relay_topic = "RelaySwich/" + chipId;
String Status = "OK"; // Status can be OK or WARNING
String Consulta = "";

struct {
  String DHTSensor_hum_name;
  String DHTSensor_temp_name;
  String TempSensor_name;
  String d0_name;
  String d1_name;
  String d2_name;
  String d3_name;
}portsNames;

struct {
  int t0;
  int t1;
  int h0;
  String d0;
  String d1;
  String d2;
  String d3;
}TemporalAccess;

/*
12860766
12860349
2415597
2447631
2462848
12885481
2365259
2383886
2384141
*/