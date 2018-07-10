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
#define SLEEP_PERIOD 20e6
#define REPEAT_EVERY_1000ms 1000
#define REPEAT_EVERY_50ms 50
#define TASK_RUN_TWICE 2

const char* ssid = "(-_-)";
const char* password = "monteiro";

/**** cloudmqtt.com ****
const char* mqtt_server = "m20.cloudmqtt.com";
const char* mqtt_user = "lgnnlude";
const char* mqtt_password = "iWg6ghr1pMLO";
const int   mqtt_port = 17283;
*/

//**** thingspeak.com ****
const char* mqtt_server = "mqtt.thingspeak.com";
const char* mqtt_user = "jjrmonteiro";
const char* mqtt_password = "Monteir0";
const char* mqtt_topic = "channels/531644/publish/HLOWA390NM8VXY7H";
const int   mqtt_port = 1883;


long lastMsg = 0;
char msg[50];

// Callback methods prototypes
void t1Callback();
void t2Callback();
void t3Callback();
void t4Callback();

//Tasks

Task t1(REPEAT_EVERY_1000ms, TASK_FOREVER, &t1Callback);		//reexecuta ao fim de 1s
Task t2(REPEAT_EVERY_1000ms, TASK_RUN_TWICE, &t2Callback);
Task t3(REPEAT_EVERY_50ms, TASK_FOREVER, &t3Callback);
Task t4(REPEAT_EVERY_50ms, TASK_FOREVER, &t4Callback);

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
	/*
	snprintf(msg, 75, "Voltage: %ld", ESP.getVcc());
	client.publish("ClientID", msg);
	snprintf(msg, 75, "Signal: %ld", WiFi.RSSI());
	client.publish("outTopic", msg);
	snprintf(msg, 75, "RAM: %ld", ESP.getFreeHeap());
	client.publish("outTopic", msg);
	*/

	String payload = "field1=";
	payload += String((float)ESP.getVcc() / 1024);
	payload += "&field2=";
	payload += String((float)ESP.getFreeHeap() / 1024);
	payload += "&field3=";
	payload += WiFi.RSSI();
	payload += "&status=MQTTPUBLISH";

	client.publish(mqtt_topic, payload.c_str());

	//*** MQTT SUBSCRIBE //***

	client.subscribe("inTopic");
}

void t2Callback() {
	Serial.print("t2: ");
	Serial.println(millis());
	
	if (t2.isLastIteration()) {
		Serial.println("t2: terminating and going to sleep.");
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

void callback(char* topic, byte* payload, unsigned int length) {///////////////////// aqui vai ficar o CLI //////////////
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");
	for (int i = 0; i < length; i++) {
		Serial.print((char)payload[i]);
	}
	Serial.println();

	// Switch on the LED if an 1 was received as first character
	if ((char)payload[0] == '1') {
		digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
		Serial.println("Server status: ON");
		// but actually the LED is on; this is because
		// it is acive low on the ESP-01)

	}
	else {
		digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
		Serial.println("Server status: OFF");

	}

}

void reconnect() {
	// Loop until we're reconnected
	//if (!client.connected()) {
		Serial.print("Attempting MQTT connection...");
		// Create a random client ID
		String clientId = "ESP8266Client-";
		clientId += String(random(0xffff), HEX);
		// Attempt to connect
		if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
			Serial.println("Connected to MQTT server!");
		}
		else {
			Serial.print("Connection failed, rc=");
			Serial.println(client.state());
			t1.disable();
		}
	//}
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

	client.loop();
	runner.execute();
}