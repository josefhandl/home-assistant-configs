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
        Serial.print( mySwitch.getReceivedValue() );
        Serial.print(" / ");
        Serial.print( mySwitch.getReceivedBitlength() );
        Serial.print("bit ");
        Serial.print("Protocol: ");
        Serial.print( mySwitch.getReceivedProtocol() );
        Serial.print(" / ");
        Serial.println( mySwitch.getReceivedDelay() );

        mySwitch.resetAvailable();
    }
}
