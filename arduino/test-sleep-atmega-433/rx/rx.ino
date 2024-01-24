// 433 MHz Přijímač

// připojení knihovny
#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();


void setup()
{
    // inicializace komunikace po sériové lince
    Serial.begin(9600);

    mySwitch.enableReceive(1);

    // nastavení typu bezdrátové komunikace
    //vw_set_ptt_inverted(true);
    // nastavení rychlosti přenosu v bitech za sekundu
    //vw_setup(1000);
    // nastavení čísla datového pinu pro přijímač
    //vw_set_rx_pin(7);
    // nastartování komunikace po nastaveném pinu
    //vw_rx_start();
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
   uint32_t binaryNum[32]; // Assuming 32 bit integer.
   int i=0;
   
   for ( ;num > 0; ){
      binaryNum[i++] = num % 2;
      num /= 2;
   }
   
   // Printing array in reverse order.
   for (int j = i-1; j >= 0; j--)
      Serial.print(binaryNum[j]);
}

void loop()
{
    // vytvoření proměnných pro uložení
    // přijaté zprávy a její délky,
    // délka je maximálně 78 znaků
    //uint8_t zprava[VW_MAX_MESSAGE_LEN];
    //uint8_t delkaZpravy = VW_MAX_MESSAGE_LEN;

    // v případě přijetí zprávy se vykoná tato if funkce
    if (mySwitch.available()) { // vw_get_message(zprava, &delkaZpravy)
        Serial.print("Received ");
        uint32_t message = mySwitch.getReceivedValue();
        decimalToBinary(message);
        Serial.print(" / ");
        Serial.print( mySwitch.getReceivedBitlength() );
        Serial.print("bit ");
        Serial.print("Protocol: ");
        Serial.print( mySwitch.getReceivedProtocol() );
        Serial.print(" / ");
        Serial.println( mySwitch.getReceivedDelay() );

        mySwitch.resetAvailable();

        uint8_t sensorId   = message >> 24 &    0b11111111; //  8 bits, 24 left
        uint16_t sleeps    = message >> 13 & 0b11111111111; // 11 bits, 13 left
        uint8_t capVoltage = message >>  7 &      0b111111; //  6 bits,  7 left
        uint16_t moisture  = message       &     0b1111111; //  7 bits,  0 left
        Serial.print("ID:");
        Serial.println(sensorId);
        Serial.print("Uptime:");
        Serial.println(sleeps);
        Serial.print("CapVoltage:");
        Serial.println(capVoltage);
        Serial.print("Moisture:");
        Serial.println(moisture);
    }
}