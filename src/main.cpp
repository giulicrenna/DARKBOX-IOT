/**
 * @file main.cpp
 * @author Giuliano Crenna (giulicrenna@gmail.com)
 * @brief
 * @version 0.1
 * @date 2022-09-02
 *
 * @copyright Copyright (c) 2022
 *
 */

// SilFe2655 darkflow-2296876
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <vector>
#include <cstdlib>
#include <WiFiManager.h>
#include <Preferences.h>
#include <LittleFS.h>

#include "global.hpp"
#include "functions.hpp"
#ifdef AP_CUSTOM_PORTAL
#include "apMode.hpp"
#endif
#ifdef SMTP_CLIENT
#include "smtp.hpp"
#endif
#include "dataSensors.hpp"
#ifdef I2C
#include "screenController.hpp"
#endif
#ifdef LOCAL_DASHBOARD
#include "httpServer.hpp"
#endif
#include "inputController.hpp"
#include "jsonizer.hpp"
#include "myMqtt.hpp"

String makeJSON(int typeOfValues, String dataExtra = "");
String makeRelayJSON();
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
void readFile(fs::FS &fs, const char *path);
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);
void renameFile(fs::FS &fs, const char *path1, const char *path2);
void deleteFile(fs::FS &fs, const char *path);
void loadData(fs::FS &fs, const char *path);
void callback(char *topic, byte *payload, unsigned int lenght);
void loadTemporalData();
void loadDataPreferences();
void refreshScreen();
void checkConn();

// Constructor for the sensors, the wifi and the MQTT Object
#ifdef AP_CUSTOM_PORTAL
apMode apInstance;
#else
WiFiManager myManager;
#endif
WiFiClient espClient;
dataSensors mySensors;
#ifdef I2C
Screen myScreen;
#endif
inputController myInputs;
JSONIZER jsonSession;

void task()
{
	timeClient.update();
	switch (currentState)
	{
	case DNS_UPDATE:
	{
		// HTTP and mDNS loop
		setupHttpServer();
		currentState = SCREEN_REFRESH;
		break;
	}
	case SCREEN_REFRESH:
	{
#ifdef I2C
		refreshScreen();
#endif
		currentState = TEMPORAL_DATA;
		break;
	}
	case TEMPORAL_DATA:
	{
		//  Temporal data to EEPROM
		if (millis() - previousTimeTemporalData >= temporalDataRefreshTime)
		{
			loadTemporalData();
			previousTimeTemporalData = millis();
		}

		/*
		Send messages by all conditions:
		- By time
		- By trigger up
		- By trigger down
		*/
		myInputs.readAllInputsbyAllConditions();

		currentState = CHECK_STATUS;
		break;
	}
	case CHECK_STATUS:
	{
		int signal = WiFi.RSSI();

		if (signal < -90)
		{
			Status = "Warning";
			Consulta = "Extremely weak signal";
		}
		else if (-55 > signal || signal >= -67)
		{
			Status = "Warning";
			Consulta = "Weak signal";
		}
		else
		{
			Status = "Ok";
			Consulta = "";
		}

		currentState = MQTT_DHT;
		break;
	}

	case MQTT_DHT:
	{
		// MQTT DHT
		if (millis() - previousTimeMQTT_DHT > MQTTDHT)
		{
			// JSON data creation
			String data_0 = makeJSON(0);
			// Serial.println(dataPretty_0.c_str());
			mqttOnLoop(host.c_str(),
					   port,
					   root_topic_publish.c_str(),
					   data_0);

			previousTimeMQTT_DHT = millis();
		}

		currentState = MQTT_SINGLE_TEMP;
		break;
	}
	case MQTT_SINGLE_TEMP:
	{
		// MQTT Sigle temperature
		if (millis() - previousMQTTsingleTemp > MQTTsingleTemp)
		{
			// JSON data creation
			String sensorData = mySensors.singleSensorRawdataTemp(0);
			
			if (sensorData != "None")
			{
				String data = makeJSON(1, sensorData);
				mqttOnLoop(host.c_str(),
						   port,
						   root_topic_publish.c_str(),
						   data);
				previousMQTTsingleTemp = millis();
			}
		}
		currentState = MQTT_KEEP_ALIVE;
		break;
	}
	case MQTT_KEEP_ALIVE:
	{
		// Keep alive message
		if (millis() - previousKeepAliveTime > keepAliveTime)
		{
			String data = makeJSON(2);
			mqttOnLoop(host.c_str(),
					   port,
					   keep_alive_topic_publish.c_str(),
					   data);
			previousKeepAliveTime = millis();
		}
		currentState = MQTT_SEND_RELAY_STATUS;
		break;
	}
	case MQTT_SEND_RELAY_STATUS:
	{
		String relayData = makeRelayJSON();
		
		// Keep alive message
		if (millis() - previousReleStatusSendTime > releStatusSendTime)
		{
			String data = makeRelayJSON();
			mqttOnLoop(host.c_str(),
					   port,
					   root_topic_publish.c_str(),
					   data);
			previousReleStatusSendTime = millis();
		}

		currentState = MQTT_POLL;
		break;
	}
	case MQTT_POLL:
	{
		mqttClient.poll();
		currentState = CONN_CHECK;
		break;
	}
	case CONN_CHECK:
	{
		checkConn();
		currentState = DNS_UPDATE;
	}
	default:
		break;
	}
}

void setup()
{
	// Serial setup
	Serial.begin(115000);
// Load and visualize data
#ifdef PREFERENCES
	// listDir(LittleFS, "/", 1);
	loadDataPreferences();
#endif
#ifndef PREFERENCES
	LittleFS.begin();
	readFile(LittleFS, "/config.json");
	loadData(LittleFS, "/config.json");
	LittleFS.end();
#endif
	// AP setup
	apInstance.setupServer();
	DHCPtoStatic(staticIpAP, gatewayAP, subnetMaskAP);
	// File System and configuration setup
	if (!LittleFS.begin())
	{
		Serial.println("(Setup Instance) SPIFFS Mount Failed");
	}
	// Devices setup
	mySensors.sensorsSetup();
#ifdef I2C
	myScreen.screenSetup();
	// Screen
	myScreen.screenClean();
	myScreen.printScreen("Starting device...", 0, 1, true);
#endif
	myInputs.inputSetup();
	// MQTT
	mqttSetup(host.c_str(),
			  port,
			  root_topic_publish.c_str(),
			  espClient,
			  keep_alive_topic_publish.c_str());
// Local Dashboard
#ifdef LOCAL_DASHBOARD
	setupServer();
#endif
}

void loop()
{
	task();
}

String makeJSON(int typeOfValues, String dataExtra)
{
	String data;
	DynamicJsonDocument dataJson(1024);
	dataJson["DeviceId"] = chipId;
	dataJson["DeviceName"] = deviceName.c_str();
	dataJson["Timestamp"] = ntpRaw();
	dataJson["Version"] = "1.0";
	switch (typeOfValues)
	{
		// MQTT DHT temp
	case 0:
	{
		dataJson["Value"][0]["Port"] = portsNames.DHTSensor_temp_name;
		dataJson["Value"][0]["Value"] = mySensors.singleSensorRawdataDHT(false);
		dataJson["Value"][1]["Port"] = portsNames.DHTSensor_hum_name;
		dataJson["Value"][1]["Value"] = mySensors.singleSensorRawdataDHT(true);
		break;
	}
	case 1:
	{
		dataJson["Value"][0]["Port"] = portsNames.TempSensor_name;
		dataJson["Value"][0]["Value"] = dataExtra;
		break;
	}
	case 2:
	{
		dataJson["Conexion"] = "WiFi";
		dataJson["Senial"] = WiFi.RSSI();
		dataJson["Battery"] = 100;
		dataJson["RedId"] = localIP;
		dataJson["Status"] = Status;
		dataJson["Consulta"] = Consulta;
		break;
	}
	}
	serializeJson(dataJson, data);
	return data;
}

String makeRelayJSON()
{
	String data;
	DynamicJsonDocument dataJson(1024);
	dataJson["DeviceId"] = chipId;
	dataJson["DeviceName"] = deviceName.c_str();
	dataJson["Timestamp"] = ntpRaw();
	dataJson["rele"] = digitalRead(Output1);

	serializeJson(dataJson, data);
	return data;
}

void checkConn()
{
	if (millis() - previousTimeTemporalCheckConnection >= 30000)
	{
		bool conn = WiFi.isConnected();
		if (!conn)
		{
			ESP.restart();
		}
		previousTimeTemporalCheckConnection = millis();
	}
}

/**
 * @brief This function load a temporal file
 * called temp.json wich is readed by the HTML and JS methods to
 * take the sensors data.
 * @param fs file system
 * @param t0 temperature 0
 * @param t1 temperature 1
 * @param h0 humidity 0
 * @param d0 digital io 0
 * @param d1 digital io 1
 * @param d2 digital io 2
 * @param d3 digital io 3
 */
void loadTemporalData()
{
	TemporalAccess.t0 = std::atoi(mySensors.singleSensorRawdataTemp(0).c_str());
	TemporalAccess.t1 = std::atoi(mySensors.singleSensorRawdataDHT(false).c_str());
	TemporalAccess.h0 = std::atoi(mySensors.singleSensorRawdataDHT(true).c_str());
	TemporalAccess.d0 = myInputs.returnSingleInput(16).c_str();
	TemporalAccess.d1 = myInputs.returnSingleInput(14).c_str();
	TemporalAccess.d2 = myInputs.returnSingleInput(12).c_str();
	TemporalAccess.d3 = myInputs.returnSingleInput(13).c_str();
	if(digitalRead(Output1) == 0){
		releStatus = "LOW";
	}else{
		releStatus="HIGH";
	}
}

#ifdef I2C
void refreshScreen()
{
	unsigned long currentTime = millis();
	// I will use millis to create the time trigger to refresh the screen
	tempString0 += "Sensor0: " + mySensors.singleSensorRawdataTemp(0);
	tempString1 += "Sensor1: " + mySensors.singleSensorRawdataTemp(1);
	tempString2 += "Sensor2: " + mySensors.singleSensorRawdataTemp(2);

	if (currentTime - previousTimeScreen >= eventInterval)
	{
		myScreen.printScreen("SENSORS DATA:", 0, 0, false);
		myScreen.printScreen(formatedTime(), 15, 0, false);
		myScreen.printScreen(tempString0, 0, 1, false);
		myScreen.printScreen(tempString1, 0, 2, false);
		myScreen.printScreen(tempString2, 0, 3, false);
	}
	tempString0 = "";
	tempString1 = "";
	tempString2 = "";
	previousTimeScreen = currentTime;
}
#endif

#ifdef PREFERENCES
void loadDataPreferences()
{
	myPref.begin("EPM", false);
	deviceName = myPref.getString("deviceName", "default");

	staticIpAP = myPref.getString("staticIpAP", "");
	subnetMaskAP = myPref.getString("subnetMaskAP", "");
	gatewayAP = myPref.getString("gatewayAP", "");

	SmtpSender = myPref.getString("SmtpSender", "default@outlook.com");
	SmtpPass = myPref.getString("SmtpPass", "default123");
	SmtpReceiver = myPref.getString("SmtpReceiver", "default@outlook.com");
	SmtpServer = myPref.getString("SmtpServer", "smtp.default.com");
	SmtpPort = std::stoi(myPref.getString("SmtpPort", "587").c_str());

	IO_0 = myPref.getString("IO_0", "10000");
	IO_1 = myPref.getString("IO_1", "10000");
	IO_2 = myPref.getString("IO_2", "10000");
	IO_3 = myPref.getString("IO_3", "10000");

	host = myPref.getString("mqtt_host", "broker.emqx.io");
	port = std::stoll(myPref.getString("mqtt_port", "1883").c_str());
	mqtt_username = myPref.getString("mqtt_username", "");
	mqtt_password = myPref.getString("mqtt_password", "");

	portsNames.DHTSensor_hum_name = myPref.getString("DHTSensor_hum_name", "A1");
	portsNames.DHTSensor_temp_name = myPref.getString("DHTSensor_temp_name", "A2");
	portsNames.TempSensor_name = myPref.getString("TempSensor_name", "A3");
	portsNames.d0_name = myPref.getString("d0_name", "D0");
	portsNames.d1_name = myPref.getString("d1_name", "D1");
	portsNames.d2_name = myPref.getString("d2_name", "D2");
	portsNames.d3_name = myPref.getString("d3_name", "D3");

	MQTTDHT = std::stoll(myPref.getString("MQTTDHT", "5000").c_str());
	MQTTsingleTemp = std::stoll(myPref.getString("MQTTsingleTemp", "5000").c_str());
	releStatusSendTime = std::stoll(myPref.getString("releStatusSendTime", "5000").c_str());
	keepAliveTime = std::stoll(myPref.getString("keepAliveTime", "120000").c_str());
	
	myPref.end();

#ifdef DEBUG
	Serial.print("\n#####################################################################");
	Serial.print("\n## Current Config:\n## Device Name: " + deviceName);
	Serial.print("\n## UID: " + String(ESP.getChipId()));
	Serial.print("\n## IP: " + staticIpAP);
	Serial.print("\n## Subnet: " + subnetMaskAP);
	Serial.print("\n## Gateway: " + gatewayAP);
	Serial.print("\n## IO0 config: " + IO_0);
	Serial.print("\n## IO1 config: " + IO_1);
	Serial.print("\n## IO2 config: " + IO_2);
	Serial.print("\n## IO3 config: " + IO_3);
	Serial.print("\n## IO0 name: " + portsNames.d0_name);
	Serial.print("\n## IO1 name: " + portsNames.d1_name);
	Serial.print("\n## IO2 name: " + portsNames.d2_name);
	Serial.print("\n## IO3 name: " + portsNames.d3_name);
	Serial.print("\n## MQTT host: " + host);
	Serial.print("\n## MQTT port: " + String(port));
	Serial.print("\n## MQTT username: " + mqtt_username);
	Serial.print("\n## MQTT password: " + mqtt_password);
	Serial.print("\n## DHT humidity name: " + portsNames.DHTSensor_hum_name);
	Serial.print("\n## DHT temperature name: " + portsNames.DHTSensor_temp_name);
	Serial.print("\n## Temperature sensor name: " + portsNames.TempSensor_name);
	Serial.print("\n## DHT sensor sending time: " + String(MQTTDHT));
	Serial.print("\n## Single sensor sending time: " + String(MQTTsingleTemp));
	Serial.print("\n## Rele status sending time: " + String(releStatusSendTime));
	Serial.println("\n## Keep alive sending time: " + String(keepAliveTime));
	Serial.println("#####################################################################");
#endif
}

#endif
/**
 * @brief
 *
 * @param fs
 * @param dirname
 * @param levels
 */
void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
	Serial.printf("Listing directory: %s\r\n", dirname);

	File root = fs.open(dirname, "r");
	if (!root)
	{
		Serial.println("- failed to open directory");
		return;
	}
	if (!root.isDirectory())
	{
		Serial.println(" - not a directory");
		return;
	}

	File file = root.openNextFile();
	while (file)
	{
		if (file.isDirectory())
		{
			Serial.print("  DIR : ");
			Serial.println(file.name());
			if (levels)
			{
				listDir(fs, file.name(), levels - 1);
			}
		}
		else
		{
			Serial.print("  FILE: ");
			Serial.print(file.name());
			Serial.print("\tSIZE: ");
			Serial.println(file.size());
		}
		file = root.openNextFile();
	}
}

#ifndef PREFERENCES

void readFile(fs::FS &fs, const char *path)
{
	Serial.printf("Reading file: %s\r\n", path);

	File file = fs.open(path, "r");
	if (!file || file.isDirectory())
	{
		Serial.println("...failed to open file for reading");
		return;
	}

	Serial.println("...read from file:");
	while (file.available())
	{
		Serial.write(file.read());
	}
	Serial.println("");

	file.close();
}

void loadData(fs::FS &fs, const char *path)
{
	File file_ = fs.open(path, "r");
	String content;
	if (!file_.available())
	{
		Serial.println("Couldn't open the file");
	}
	while (file_.available())
	{
		content += file_.readString();
		break;
	}
	StaticJsonDocument<1024> config;
	auto error = deserializeJson(config, content);

	if (error)
	{
		Serial.println("Failed to deserialize");
		Serial.println(error.f_str());
		restoreConfig(LittleFS);
	}

	// Serial.println(config.size());

	deviceName = (const char *)config["device"]["name"];

	staticIpAP = (const char *)config["network"]["ip"];
	subnetMaskAP = (const char *)config["network"]["subnetMask"];
	gatewayAP = (const char *)config["network"]["gateway"];

	SmtpSender = (const char *)config["smtp"]["mailSender"];
	SmtpPass = (const char *)config["smtp"]["mailPassword"];
	SmtpReceiver = (const char *)config["smtp"]["mailReceiver"];
	SmtpServer = (const char *)config["smtp"]["smtpServer"];
	SmtpPort = std::stoi((const char *)config["smtp"]["smtpPort"]);

	IO_0 = (const char *)config["ports"]["IO_0"];
	IO_1 = (const char *)config["ports"]["IO_1"];
	IO_2 = (const char *)config["ports"]["IO_2"];
	IO_3 = (const char *)config["ports"]["IO_3"];

	portsNames.DHTSensor_hum_name = String((const char *)config["names"]["DHTSensor_hum_name"]);
	portsNames.DHTSensor_temp_name = String((const char *)config["names"]["DHTSensor_temp_name"]);
	portsNames.TempSensor_name = String((const char *)config["names"]["TempSensor_name"]);
	portsNames.d0_name = String((const char *)config["names"]["d0_name"]);
	portsNames.d1_name = String((const char *)config["names"]["d1_name"]);
	portsNames.d2_name = String((const char *)config["names"]["d2_name"]);
	portsNames.d3_name = String((const char *)config["names"]["d3_name"]);

	MQTTDHT = std::stoi((const char *)config["etc"]["DHT"]);
	MQTTsingleTemp = std::stoi((const char *)config["etc"]["SingleTemp"]);
	keepAliveTime = std::stoi((const char *)config["etc"]["keepAlive"]);

	Serial.println("#### CONFIG LOADED ####");

	file_.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
	Serial.printf("Writing file: %s\r\n", path);

	File file = fs.open(path, "w");
	if (!file)
	{
		Serial.println("...failed to open file for writing");
		return;
	}
	if (file.print(message))
	{
		Serial.println("...file written");
	}
	else
	{
		Serial.println("...write failed");
	}
	file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2)
{
	Serial.printf("Renaming file %s to %s\r\n", path1, path2);
	if (fs.rename(path1, path2))
	{
		Serial.println("...file renamed");
	}
	else
	{
		Serial.println("...rename failed");
	}
}

void deleteFile(fs::FS &fs, const char *path)
{
	Serial.printf("Deleting file: %s\r\n", path);
	if (fs.remove(path))
	{
		Serial.println("...file deleted");
	}
	else
	{
		Serial.println("...delete failed");
	}
}

void testFileIO(fs::FS &fs, const char *path)
{
	Serial.printf("Testing file I/O with %s\r\n", path);

	static uint8_t buf[512];
	size_t len = 0;
	File file = fs.open(path, "w");
	if (!file)
	{
		Serial.println("...failed to open file for writing");
		return;
	}

	size_t i;
	Serial.print("...writing");
	uint32_t start = millis();
	for (i = 0; i < 2048; i++)
	{
		if ((i & 0x001F) == 0x001F)
		{
			Serial.print(".");
		}
		file.write(buf, 512);
	}
	Serial.println("");
	uint32_t end = millis() - start;
	Serial.printf(" - %u bytes written in %u ms\r\n", 2048 * 512, end);
	file.close();

	file = fs.open(path, "r");
	start = millis();
	end = start;
	i = 0;
	if (file && !file.isDirectory())
	{
		len = file.size();
		size_t flen = len;
		start = millis();
		Serial.print("...reading");
		while (len)
		{
			size_t toRead = len;
			if (toRead > 512)
			{
				toRead = 512;
			}
			file.read(buf, toRead);
			if ((i++ & 0x001F) == 0x001F)
			{
				Serial.print(".");
			}
			len -= toRead;
		}
		Serial.println("");
		end = millis() - start;
		Serial.printf("- %u bytes read in %u ms\r\n", flen, end);
		file.close();
	}
	else
	{
		Serial.println("...failed to open file for reading");
	}
}

#endif