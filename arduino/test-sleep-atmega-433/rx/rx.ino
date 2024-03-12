
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


const uint8_t pin_infoLed = 7;


uint8_t messageArray[RH_ASK_MAX_MESSAGE_LEN];
uint8_t messageArrayLen = sizeof(messageArray);



void blink(const uint8_t count) {
    for (uint8_t i = 0; i < count; ++i) {
        digitalWrite(pin_infoLed, HIGH);
        delay(100);
        digitalWrite(pin_infoLed, LOW);
        delay(100);
    }
}

uint32_t messageapi = 0;

void handleRoot() {
  server.send(200, "text/plain", "hello from pico w!\n");
  blink(1);
}

void handleSensors() {
  char snum[33];
  itoa(messageapi, snum, 2);
  server.send(200, "text/plain", snum);
  blink(1);
}


void setup()
{
    pinMode(pin_infoLed, OUTPUT);
    digitalWrite(pin_infoLed, HIGH);


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

    digitalWrite(pin_infoLed, LOW);
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

void parseMessage() {
        uint32_t message = 0;

        message |= ((uint32_t)messageArray[0] << 24);
        message |= ((uint32_t)messageArray[1] << 16);
        message |= ((uint32_t)messageArray[2] << 8);
        message |= (uint32_t)messageArray[3];

        Serial.print("Received ");
        Serial.print(message);
        Serial.print(" ");
        decimalToBinary(message);

        messageapi = message;

        uint8_t sensorId   = message >> 24 &    0b11111111; //  8 bits, 24 left
        uint16_t sleeps    = message >> 13 & 0b11111111111; // 11 bits, 13 left
        uint8_t capVoltage = message >>  7 &      0b111111; //  6 bits,  7 left
        uint16_t moisture  = message       &     0b1111111; //  7 bits,  0 left
        Serial.print("ID: ");
        Serial.println(sensorId);
        Serial.print("Uptime: ");
        Serial.println(sleeps);
        Serial.print("CapVoltage: ");
        Serial.println(capVoltage);
        Serial.print("Moisture: ");
        Serial.println(moisture);
}

void loop()
{
    server.handleClient();

    if (driver.recv(messageArray, &messageArrayLen)) {
        parseMessage();
        blink(2);
    }
}
