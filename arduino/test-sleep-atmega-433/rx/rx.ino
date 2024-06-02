
#include <RH_ASK.h>
#ifdef RH_HAVE_HARDWARE_SPI
#include <SPI.h> // Not actually used but needed to compile
#endif

#include <WiFi.h>
#include <WebServer.h>

#ifndef STASSID
#define STASSID ""
#define STAPSK ""
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

WiFiMulti multi;
WebServer server(80);

// Atmega328p (for 6000 tx on attiny85 8 MHz)
//RH_ASK driver(747, 11, -1, -1);

// RP2040 (for 6000 tx on attiny85 8 MHz)
RH_ASK driver(738, 2, -1, -1);


const uint8_t pin_ledRed   = 7;
const uint8_t pin_ledGreen = 8;
const uint8_t pin_ledBlue  = 9;
const uint8_t pin_ledArray[] = {pin_ledRed, pin_ledGreen, pin_ledBlue};

const uint8_t color_orange[] = {255, 40, 0};
const uint8_t color_purple[] = {255, 0, 40};
const uint8_t color_cyan[]   = {0, 255, 20};


uint8_t messageFragments[RH_ASK_MAX_MESSAGE_LEN];
uint8_t messageFragmentsLen = sizeof(messageFragments);

struct Sensor {
    uint16_t sleeps = 0;
    uint8_t capVoltage = 0;
    uint16_t moisture = 0;
};


Sensor sensorsData[256];


void ledOn(const uint8_t * color) {
    analogWrite(pin_ledRed,   color[0]);
    analogWrite(pin_ledGreen, color[1]);
    analogWrite(pin_ledBlue,  color[2]);
}

void ledOff() {
    digitalWrite(pin_ledRed,   LOW);
    digitalWrite(pin_ledGreen, LOW);
    digitalWrite(pin_ledBlue,  LOW);
}

void blink(const uint8_t count, const uint8_t * color) {
    for (uint8_t i = 0; i < count; ++i) {
        ledOn(color);
        delay(100);
        ledOff();
        delay(100);
    }
}

void handleRoot() {
    server.send(200, "text/plain", "hello from pico w!\n");
    blink(1, color_orange);
}

void handleSensors() {
    char result[1024];
    strcpy(result, "{\"sensors\":[");
    bool firstSensor = true;
    for (uint16_t sensorId = 0; sensorId < 256; ++sensorId) {
        if (sensorsData[sensorId].sleeps > 0) {
            Sensor & sensor = sensorsData[sensorId];
            char sensorResult[256];
            snprintf(sensorResult, sizeof(sensorResult), "{\"id\":%u,\"sleeps\":%u,\"capVoltage\":%u,\"moisture\":%u}", sensorId, sensor.sleeps, sensor.capVoltage, sensor.moisture);
            if (!firstSensor) {
                strcat(result, ",");  // Add comma before each sensor data except the first one
            }

            // Ensure concatenation does not overflow the buffer
            if (strlen(result) + strlen(sensorResult) < sizeof(result)) {
                strcat(result, sensorResult);
                firstSensor = false;
            } else {
                // Handle buffer overflow: you can log an error or handle it as needed
                blink(5, color_orange);  // Indicate an error with blinking
                return;
            }
        }
    }
    strcat(result, "]}");

    blink(1, color_orange);
    server.send(200, "application/json", result);
}


void setup()
{
    // Init LEDs
    pinMode(pin_ledRed,   OUTPUT);
    pinMode(pin_ledGreen, OUTPUT);
    pinMode(pin_ledBlue,  OUTPUT);

    // Turn boot color on
    ledOn(color_cyan);


    Serial.begin(9600);

    if (!driver.init())
#ifdef RH_HAVE_SERIAL
      Serial.println("init failed");
#else
      ;
#endif

    delay(1000);
    Serial.print("booting....");
    delay(100);
    Serial.println("done");

    Serial.print("Connecting to ");
    Serial.println(ssid);

    multi.addAP(ssid, password);

    if (multi.run() != WL_CONNECTED) {
      Serial.println("Unable to connect to network, rebooting in 10 seconds...");
      delay(10000);
      rp2040.reboot();
    }

    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    Serial.println("-------------------------");

    server.on("/", handleRoot);
    server.on("/sensors", handleSensors);
    server.begin();

    ledOff();
}

void decimalToBinary(uint32_t num) {   
    if (num == 0) {
        Serial.print("0");
        return;
    }

    // Stores binary representation of number.
    uint8_t binaryNum[32]; // Assuming 32 bit integer.
    int i=0;

    for ( ;num > 0; ){
      binaryNum[i++] = num % 2;
      num /= 2;
    }

    // Printing array in reverse order.
    for (int j = i-1; j >= 0; j--)
      Serial.print(binaryNum[j]);

    Serial.println();
}

void printSerialSensor(const uint8_t sensorId, const uint32_t fullMessage) {
    Serial.print("Received ");
    Serial.print(fullMessage);
    Serial.print(" ");
    decimalToBinary(fullMessage);
    Serial.print("ID: ");
    Serial.println(sensorId);
    Serial.print("Uptime: ");
    Serial.println(sensorsData[sensorId].sleeps);
    Serial.print("CapVoltage: ");
    Serial.println(sensorsData[sensorId].capVoltage);
    Serial.print("Moisture: ");
    Serial.println(sensorsData[sensorId].moisture);
}

void loop()
{
    server.handleClient();

    if (driver.recv(messageFragments, &messageFragmentsLen)) {
        // Join fragments to the whole message
        uint32_t message = 0;
        message |= ((uint32_t)messageFragments[0] << 24);
        message |= ((uint32_t)messageFragments[1] << 16);
        message |= ((uint32_t)messageFragments[2] << 8);
        message |= (uint32_t)messageFragments[3];

        // Get sensor ID and the corresponding Sensor struct
        uint8_t sensorId  = message >> 24 &    0b11111111; //  8 bits, 24 left
        Sensor & sensor = sensorsData[sensorId];

        // Parse rest of of the message and save
        sensor.sleeps     = message >> 13 & 0b11111111111; // 11 bits, 13 left
        sensor.capVoltage = message >>  7 &      0b111111; //  6 bits,  7 left
        sensor.moisture   = message       &     0b1111111; //  7 bits,  0 left

        blink(2, color_purple);

        // Print to serial console
        printSerialSensor(sensorId, message);
    }
}
