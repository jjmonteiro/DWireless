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

const char* ssid = "(-_-)";
const char* password = "monteiro";

//**** cloudmqtt.com ****
const char* mqtt_server = "m20.cloudmqtt.com";
const char* mqtt_user = "lgnnlude";
const char* mqtt_password = "iWg6ghr1pMLO";
const int   mqtt_port = 17283;
String MQTT_ID, topic, payload;


/**** thingspeak.com ****
const char* mqtt_server = "mqtt.thingspeak.com";
const char* mqtt_user = "jjrmonteiro";
const char* mqtt_password = "Monteir0";
const char* mqtt_topic = "channels/531644/publish/HLOWA390NM8VXY7H";
const int   mqtt_port = 1883;*/


long lastMsg = 0;
char msg[50];

// Callback methods prototypes
void t1Callback();
void t2Callback();
void t3Callback();
void t4Callback();

//Tasks

Task t1(2000, TASK_FOREVER, &t1Callback);		//re-run after 2sec, forever
Task t2(1000, TASK_RUN_TWICE, &t2Callback);
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

	topic = MQTT_ID + "/Voltage";
	payload = String(ESP.getVcc());
	Serial.println(client.publish(topic.c_str(), payload.c_str(), true));
	topic = MQTT_ID + "/Signal";
	payload = String(WiFi.RSSI());
	Serial.println(client.publish(topic.c_str(), payload.c_str(), true));
	topic = MQTT_ID + "/FreeHeap";
	payload = String(ESP.getFreeHeap());
	Serial.println(client.publish(topic.c_str(), payload.c_str(), true));
	

	/*** thingspeak
	String payload = "field1=";
	payload += String((float)ESP.getVcc() / 1024);
	payload += "&field2=";
	payload += String((float)ESP.getFreeHeap() / 1024);
	payload += "&field3=";
	payload += WiFi.RSSI();
	payload += "&status=MQTTPUBLISH";
	client.publish(mqtt_topic, payload.c_str());*/

	//*** MQTT SUBSCRIBE //***
	Serial.println("Subscribing messages..");
	topic = MQTT_ID + "/Command";
	Serial.println(client.subscribe(topic.c_str()));
}

void t2Callback() {
	Serial.print("t2: ");
	Serial.println(millis());
	
	if (t2.isLastIteration()) {
		Serial.println("t2: terminating and going to sleep.");
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
	topic = MQTT_ID + "/Reply";
	payload = "Unknown command.";	//default output

	Serial.print("Last Message on Topic [");
	Serial.print(in_topic);
	Serial.print("] :: Payload [");
	Serial.print(command);
	Serial.println("]");

	Serial.print("> ");

	// Switch on the LED if an 1 was received as first character
	if (command == "server_on") {
		digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
		payload = "Server status: ON";
	}
	if (command == "server_off") {
		digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
		payload = "Server status: OFF";
	}
	if (command == "ip_addr") {
		payload = WiFi.localIP().toString();
	}
	if (command == "sleep_off") {
		t2.disable();
		payload = "Deep sleep status: OFF";
	}
	if (command == "sleep_on") {
		t2.enable();
		payload = "Deep sleep status: ON";
	}

	Serial.println(payload);
	Serial.println(client.publish(topic.c_str(), payload.c_str(), true));
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
	// String clientId = "ESP8266Client-";
	// clientId += String(random(0xffff), HEX);
	// Attempt to connect
	// boolean connect (clientID, username, password, willTopic, willQoS, willRetain, willMessage)
	topic = MQTT_ID + "/Status";
	if (client.connect(MQTT_ID.c_str(), mqtt_user, mqtt_password, topic.c_str(), 1, true, "CONNECTION_LOST")) {
		Serial.println("Connected to MQTT server!");
		Serial.println(client.publish(topic.c_str(), "CONNECTION_START", true));
	}
	else {
		Serial.print("Connection failed, rc=");
		Serial.println(client.state());
		t1.disable();
	}

}

void setup_wifi() {
	// We start by connecting to a WiFi network

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
	MQTT_ID = WiFi.macAddress();
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

	Serial.println("Initializing Pinmodes..");
	pinMode(LED_BUILTIN, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
	digitalWrite(LED_BUILTIN, HIGH);  // active low

	Serial.println("Initializing WiFi..");
	setup_wifi();
	Serial.println("Initializing MQTT Server..");
	client.setServer(mqtt_server, mqtt_port);
	client.setCallback(callback);

	Serial.println("initializing scheduler..");
	runner.init();

	Serial.println("Adding tasks..");
	runner.addTask(t1);
	runner.addTask(t2);
	runner.addTask(t3);
	runner.addTask(t4);
	Serial.println("Enabling tasks..");
	t1.enable();
	t2.enable();
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