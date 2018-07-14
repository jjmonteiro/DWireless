/*
 Name:		DWireless.ino
 Created:	01-Jul-18 02:44:00
 Author:	Joaquim Monteiro
*/

#include <EEPROM.h>
#include <TaskScheduler.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>


//define constants and vars
#define SLEEP_PERIOD 20e6	//20 seconds
#define TASK_RUN_TWICE 2
#define SENSOR "SENSOR"
#define STATUS "STATUS"

//**** NETWORK CREDENTIALS
char* ssid;
char* password;

//**** SERVER INFO [cloudmqtt.com] ****
const char* mqtt_server;
const char* mqtt_user;
const char* mqtt_password;
int mqtt_port;

//**** DEVICE IDENTIFIERS
const char* MAC;
const char* NAME;
const char* TYPE;
const char* ZONE;
String defaultTopic;


// Callback methods prototypes
void t1Callback();
void deepSleepCallback();
void t3Callback();
void t4Callback();

//Tasks

Task t1(2000, TASK_FOREVER, &t1Callback);		//re-run after 2sec, forever
Task deepSleep(1000, TASK_RUN_TWICE, &deepSleepCallback);
Task t3(50, TASK_FOREVER, &t3Callback);
Task t4(50, TASK_FOREVER, &t4Callback);

//Objects

WiFiClient espClient;
PubSubClient client(espClient);

Scheduler runner;
ADC_MODE(ADC_VCC);

void t1Callback() {
	Serial.print("t1: ");
	Serial.println(millis());

	//*** MQTT PUBLISH //***	
	Serial.println("Publishing messages..");

	//*** cloudmqtt ***
	String payload;
	int fail = 0;

	payload = String((float)ESP.getVcc()/1024);
	fail += !client.publish((defaultTopic + "/VOLTAGE").c_str(), payload.c_str(), true);

	payload = String(WiFi.RSSI());
	fail += !client.publish((defaultTopic + "/SIGNAL").c_str(), payload.c_str(), true);

	payload = String((float)ESP.getFreeHeap()/1024);
	fail += !client.publish((defaultTopic + "/MEMORY").c_str(), payload.c_str(), true);
	
	payload = String((float)ESP.getFreeHeap() / 1111);
	fail += !client.publish((defaultTopic + "/PHOTOCELL").c_str(), payload.c_str(), true);

	payload = String((float)ESP.getFreeHeap() / 2222);
	fail += !client.publish((defaultTopic + "/THERMAL").c_str(), payload.c_str(), true);

	payload = String("FALSE");
	fail += !client.publish((defaultTopic + "/ALARM").c_str(), payload.c_str(), true);

	payload = String("FALSE");
	fail += !client.publish((defaultTopic + "/FAULT").c_str(), payload.c_str(), true);

	payload = String("FALSE");
	fail += !client.publish((defaultTopic + "/DISABLED").c_str(), payload.c_str(), true);

	if (fail) {
		Serial.println("One or more publications failed.");
		fail = 0;
	}


	//*** MQTT SUBSCRIBE //***
	Serial.println("Subscribing messages..");

	fail += !client.subscribe((defaultTopic + "/REQUEST").c_str());

	if (fail) {
		Serial.println("One or more subscriptions failed.");
		fail = 0;
	}
}

void deepSleepCallback() {
	Serial.print("t2: ");
	Serial.println(millis());
	
	if (deepSleep.isLastIteration()) {
		Serial.println("t2: terminating and going to sleep.");
		client.publish((defaultTopic + "/STATUS").c_str(), "CONNECTION_CLOSE", true);
		client.disconnect();
		runner.disableAll();
		delay(200);
		WiFi.disconnect();
		ESP.deepSleep(SLEEP_PERIOD);
	}
}

void t3Callback() {
	Serial.print("t3: ");
	Serial.println(millis());
}

void t4Callback() {
	Serial.print("t4: ");
	Serial.println(millis());
}

void callback(char* in_topic, byte* in_payload, unsigned int length) {///////////////////// aqui vai ficar o CLI //////////////
	String command = PtrToString(in_payload, length);
	String payload = "Unknown command.";	//default output

	Serial.print("Last Message on Topic [");
	Serial.print(in_topic);
	Serial.print("] :: Payload [");
	Serial.print(command);
	Serial.println("]");

	Serial.print("> ");

	if (command == "server_on") {
		digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on by making the voltage LOW
		payload = "Server: ON, Deepsleep: OFF";
		deepSleep.disable();
	}
	if (command == "server_off") {
		digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
		payload = "Server: OFF, Deepsleep: ON";
		deepSleep.enable();
	}
	if (command == "ip_addr") {
		payload = WiFi.localIP().toString();
	}
	if (command == "sleep_off") {
		deepSleep.disable();
		payload = "Deepsleep: OFF";
	}
	if (command == "sleep_on") {
		deepSleep.enable();
		payload = "Deepsleep: ON";
	}
	if (command == "") {
		return;
	}

	Serial.println(payload);
	Serial.println(client.publish((defaultTopic + "/REPLY").c_str(), payload.c_str(), false));
	Serial.println(client.publish((defaultTopic + "/REQUEST").c_str(), "", false));
}

String PtrToString(uint8_t *str, unsigned int len) {
	String result;
	for (int i = 0; i < len; i++) {
		result += ((char)str[i]);
	}
	return result;
}

void reconnect() {

	Serial.println("Attempting MQTT connection...");
	// Create a random client ID
	String clientId = "ESP01_";
	clientId += String(random(0xffff), HEX);
	// Attempt to connect
	// boolean connect (clientID, username, password, willTopic, willQoS, willRetain, willMessage)
	Serial.println(clientId);
	Serial.println(mqtt_user);
	Serial.println(mqtt_password);
	Serial.println(defaultTopic + "/STATUS");

	if (client.connect(clientId.c_str(), mqtt_user, mqtt_password, (defaultTopic + "/STATUS").c_str(), 1, true, "CONNECTION_LOST")) {
		Serial.println("Connected to MQTT server!");
		client.publish((defaultTopic + "/STATUS").c_str(), "CONNECTION_START", true);
	}
	else {
		Serial.print("Connection failed, rc=");
		Serial.println(client.state());
		t1.disable();
	}

}

void setupWifi() {
	// extract wifi credentials from eeprom
	// attempt connection, if fail start autoconfiguration or WPS
	// when successfull connected, save new credentials

	Serial.print("Connecting to: ");
	Serial.println(ssid);
	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {
		yield();
	}

	//Serial.println("");
	Serial.println("WiFi connected!");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
	Serial.print("Signal: ");
	Serial.println(WiFi.RSSI());
	Serial.print("MAC address: ");
	Serial.println(WiFi.macAddress());


}

void readEepromData() {
	String sMAC;

	ssid = "(-_-)";
	password = "monteiro";

	//**** SERVER INFO ****
	mqtt_server = "m20.cloudmqtt.com";
	mqtt_user = "lgnnlude";
	mqtt_password = "iWg6ghr1pMLO";
	mqtt_port = 17283;

	NAME = "DET_021";
	TYPE = "PH850";
	ZONE = "ZONE_1";

	sMAC = WiFi.macAddress();
	sMAC.replace(":", "");
	//MAC = sMAC.toCharArray;
	defaultTopic = sMAC;
	defaultTopic += "/";
	defaultTopic += NAME;
	defaultTopic += "/";
	defaultTopic += ZONE;
}
void setup() {

	Serial.begin(115200);
	Serial.println();
	Serial.println("--Boot Startup--");
	Serial.println("Serial Initialized @ 115200");

	Serial.print("Cause: ");
	Serial.println(ESP.getResetReason());
	Serial.println("-------------");
	Serial.println(ESP.getChipId());
	Serial.println(ESP.getCoreVersion());
	Serial.println(ESP.getSdkVersion());
	Serial.println(ESP.getCpuFreqMHz());
	Serial.println(ESP.getVcc());
	Serial.println("-------------");
	Serial.println(ESP.getFreeHeap());
	Serial.println(ESP.getSketchSize());
	Serial.println(ESP.getFreeSketchSpace());
	Serial.println(ESP.getFlashChipId());
	Serial.println(ESP.getFlashChipSize());
	Serial.println(ESP.getFlashChipRealSize());
	Serial.println(ESP.getFlashChipSpeed());
	Serial.println("-------------");

	readEepromData();

	Serial.println("Initializing GPIOs..");
	pinMode(LED_BUILTIN, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
	digitalWrite(LED_BUILTIN, HIGH);  // active low

	Serial.println("Initializing WiFi..");
	setupWifi();
	Serial.println("Initializing MQTT Server..");
	client.setServer(mqtt_server, mqtt_port);
	client.setCallback(callback);

	Serial.println("initializing scheduler..");
	runner.init();

	Serial.println("Adding tasks..");
	runner.addTask(t1);
	runner.addTask(deepSleep);
	runner.addTask(t3);
	runner.addTask(t4);
	Serial.println("Enabling tasks..");
	t1.enable();
	deepSleep.enable();
	t3.enable();
	t4.enable();
	Serial.println("--Boot Complete--");
}


void loop() {

	if (!client.connected()) {
		reconnect();
	}
	yield();
	client.loop();
	runner.execute();
}