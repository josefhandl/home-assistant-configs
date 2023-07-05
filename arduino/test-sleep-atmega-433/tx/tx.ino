// 433 MHz vysílač

// připojení knihovny
#include <VirtualWire.h>

#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>

#include "SystemStatus.h"

// these define cbi and sbi, for as far they are not known yet
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

int transmitterPowerPin = 4;
int transmitterPin = 3;
int heartPin = 4;
volatile boolean f_wdt = 1;
int counter = 0;
unsigned long sleeps = 0;

SystemStatus ss;

void setup_watchdog(int ii) 
{
  // 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
  // 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec

  uint8_t bb;
  if (ii > 9 ) ii=9;
    bb=ii & 7;
  if (ii > 7) bb|= (1<<5);
  bb|= (1<<WDCE);

  MCUSR &= ~(1<<WDRF);
  // start timed sequence
  WDTCR |= (1<<WDCE) | (1<<WDE);
  // set new watchdog timeout value
  WDTCR = bb;
  WDTCR |= _BV(WDIE);
}


// system wakes up when watchdog is timed out
void system_sleep() 
{
  cbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF
  setup_watchdog(8);                   // approximately 8 seconds sleep
 
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();
  sei();                               // Enable the Interrupts so the wdt can wake us up

  sleep_mode();                        // System sleeps here

  sleep_disable();                     // System continues execution here when watchdog timed out 
  sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
}

void send_msg() {
    // vytvoření proměnných pro různé
    // druhy zpráv
    // proměnná zprava pro poslání textu
    const char *zprava = "uptime: ";
    // proměnná s načtením počtu sekund od
    // připojení napájení
    //unsigned long cas = millis(); // nevim proc, ale nefunguje
    // pracovní proměnná pro konverzi
    // čísla na text
    char znaky [64];
    // příkazy pro konverzi čísla na text,
    // čas převedený na text je uložen do
    // proměnné casZnaky
    snprintf(znaky, sizeof(znaky), "%lu", sleeps);
    char *casZnaky = znaky;


    int capVoltage = ss.getVCC();
    const char *zprava_vol = "voltage: ";

    char znaky_vol [64];
    snprintf(znaky_vol, sizeof(znaky_vol), "%d", capVoltage);
    char *casZnaky_vol = znaky_vol;


    // enable power to the 433 transmitter
    digitalWrite(transmitterPowerPin, HIGH);
    //delay(200000);
    //delay(2000);

    // rozsvícení LED diody při odesílání (nepovinné)
    //digitalWrite(heartPin, HIGH);
    // odeslání textu v proměnné zprava
    vw_send((uint8_t *)zprava, strlen(zprava));
    // vyčkání na odeslání celé zprávy
    vw_wait_tx();
    // zhasnutí LED diody při odeslání (nepovinné)
    //digitalWrite(heartPin, LOW);
    // pauza mezi posláním zpráv


    delay(100);


    // obdobný kus kódu, který opět rozsvítí LED
    // diodu, zašle obsah proměnné casZnaky
    // a po odeslání LED diodu zhasne
    //digitalWrite(heartPin, HIGH);
    vw_send((uint8_t *)casZnaky, strlen(casZnaky));
    vw_wait_tx();
    //digitalWrite(heartPin, LOW);
    delay(100);

    vw_send((uint8_t *)zprava_vol, strlen(zprava_vol));
    vw_wait_tx();
    delay(100);
    vw_send((uint8_t *)znaky_vol, strlen(znaky_vol));
    vw_wait_tx();

    // disable power to the 433 transmitter
    digitalWrite(transmitterPowerPin, LOW);
}


void setup()
{
    pinMode(heartPin, OUTPUT);
    pinMode(transmitterPin, OUTPUT);
    pinMode(transmitterPowerPin, OUTPUT);

    digitalWrite(heartPin, LOW);
    digitalWrite(transmitterPin, LOW);
    digitalWrite(transmitterPowerPin, LOW);

    // nastavení typu bezdrátové komunikace
    vw_set_ptt_inverted(true);
    // nastavení čísla datového pinu pro vysílač
    vw_set_tx_pin(transmitterPin);
    // nastavení rychlosti přenosu v bitech za sekundu
    vw_setup(1000);

    setup_watchdog(8); // approximately 4 seconds sleep
}

void loop()
{
    //if (f_wdt==1) {  // wait for timed out watchdog / flag is set when a watchdog timeout occurs
    //    f_wdt=0;       // reset flag
    //}

    if (counter > 0){ //timer to reset approx 15 min
        counter = 0;
        sleeps++;
        send_msg();
    }

    // Show heartbeat (LED blink)
    //digitalWrite(heartPin, HIGH);
    //delay(100000);
    //digitalWrite(heartPin, LOW);
    system_sleep();
}

// Watchdog Interrupt Service / is executed when watchdog timed out
ISR(WDT_vect) {
    counter++;
    f_wdt=1;  // set global flag
}