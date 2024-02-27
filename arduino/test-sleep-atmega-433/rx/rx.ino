
#include <RH_ASK.h>
#ifdef RH_HAVE_HARDWARE_SPI
#include <SPI.h> // Not actually used but needed to compile
#endif

RH_ASK driver(747, 11, -1, -1);

void setup()
{
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

void loop()
{
    uint8_t messageArray[RH_ASK_MAX_MESSAGE_LEN];
    uint8_t messageArrayLen = sizeof(messageArray);

    if (driver.recv(messageArray, &messageArrayLen)) {
        uint32_t message = 0;

        message |= ((uint32_t)messageArray[0] << 24);
        message |= ((uint32_t)messageArray[1] << 16);
        message |= ((uint32_t)messageArray[2] << 8);
        message |= (uint32_t)messageArray[3];

        Serial.print("Received ");
        decimalToBinary(message);

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
}
