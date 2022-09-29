unsigned int port = 1883;
unsigned int eventInterval = 1500;
unsigned int previousTimeScreen = 0;
unsigned int previousTimeTemporalData = 0;
unsigned int previousTimeMQTTtemp = 0;
unsigned int previousTimeMQTThum = 0;
unsigned int previousKeepAliveTime = 0;
unsigned int temporalDataRefreshTime = 10;
unsigned int MQTTtemp;
unsigned int MQTThum;
unsigned int keepAliveTime;

String chipId = String(ESP.getChipId());
String IO_0;
String IO_1;
String IO_2;
String IO_3;
String deviceName;
String staticIpAP;
String subnetMaskAP;
String gatewayAP;
String smtpSender;
String smtpPass;
String SmtpReceiver;
String SmtpServer;
String tempString0, tempString1, tempString2;
String host = "mqtt.darkflow.com.ar";
String configTopic = "DeviceConfig/" + chipId;
String root_topic_subscribe = "DeviceConfig/" + chipId;
String root_topic_publish = "DeviceData/" + chipId;
String keep_alive_topic_publish = "DeviceStatus/" + chipId;

struct {
  int t0;
  int t1;
  int h0;
  String d0;
  String d1;
  String d2;
  String d3;
}TemporalAccess;
