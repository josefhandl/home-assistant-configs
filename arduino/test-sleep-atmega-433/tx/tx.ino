// 433 MHz vysílač

// připojení knihovny
//#include <VirtualWire.h>
#include <RCSwitch.h>
RCSwitch rcSwitch = RCSwitch();


#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>

#include "SystemStatus.h"

// these define cbi and sbi, for as far they are not known yet
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

char sensorId = 170;

int transmitterPowerPin = 4; // (D4), pin 3
int transmitterPin = 2; // (D2), pin 7
int heartPin = 4;
volatile boolean f_wdt = 1;
unsigned long sleeps = 0;

int soilMoisturePin = 3; // (D3), pin 2
const int openAirReading = 590;   //calibration data 1
const int waterReading = 290;     //calibration data 2
int moistureLevel = 0;
int moisturePercentage = 0;

int counterStart = 0; // sleeps per cycle - timer to reset (63 means approx 8 min)
int counter = counterStart;

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
  setup_watchdog(9);                   // approximately 8 seconds sleep
 
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();
  sei();                               // Enable the Interrupts so the wdt can wake us up

  sleep_mode();                        // System sleeps here

  sleep_disable();                     // System continues execution here when watchdog timed out 
  sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
}

void sendMsg() {
    // enable power to the 433 transmitter
    digitalWrite(transmitterPowerPin, HIGH);

    // send uptime
    //=========================
    const char *zprava = "uptime: ";
    //unsigned long cas = millis(); // nevim proc, ale nefunguje
    char znaky [64];
    snprintf(znaky, sizeof(znaky), "%lu", sleeps);
    char *casZnaky = znaky;

    //vw_send((uint8_t *)zprava, strlen(zprava));
    //vw_wait_tx();
    delay(100);
    //vw_send((uint8_t *)casZnaky, strlen(casZnaky));
    //vw_wait_tx();
    delay(100);

    // send vcc voltage
    //==========================
    uint8_t capVoltage = ss.getVCC() / 100;
    const char *zprava_vol = "voltage: ";

    snprintf(znaky, sizeof(znaky), "%d", capVoltage);
    char *casZnaky_vol = znaky;

    //vw_send((uint8_t *)zprava_vol, strlen(zprava_vol));
    //vw_wait_tx();
    delay(100);
    //vw_send((uint8_t *)znaky, strlen(znaky));
    //vw_wait_tx();
    delay(100);

    // send moisture level
    //===========================
    moistureLevel = analogRead(soilMoisturePin);
    moisturePercentage = map(moistureLevel, openAirReading, waterReading, 0, 100);
    const char *zprava_m = "moisture: ";

    snprintf(znaky, sizeof(znaky), "%d", moisturePercentage);
    char *casZnaky_m = znaky;

    //vw_send((uint8_t *)zprava_m, strlen(zprava_m));
    //vw_wait_tx();
    delay(100);
    //vw_send((uint8_t *)znaky, strlen(znaky));
    //vw_wait_tx();
    uint32_t message = (uint32_t)sensorId << 24; // 24 bits left
    message |= (uint32_t)sleeps << 13; // (=11 bits), << 11 later (= 13 bits)
    message |= (uint32_t)capVoltage << 7; // 7 bits left
    message |= moisturePercentage & 0b1111111;
    rcSwitch.send(message, 32);

    delay(100);

    // disable power to the 433 transmitter
    digitalWrite(transmitterPowerPin, LOW);
}


void setup()
{
    pinMode(heartPin, OUTPUT);
    pinMode(transmitterPin, OUTPUT);
    pinMode(transmitterPowerPin, OUTPUT);
    pinMode(soilMoisturePin, INPUT);

    digitalWrite(heartPin, LOW);
    digitalWrite(transmitterPin, LOW);
    digitalWrite(transmitterPowerPin, LOW);

    rcSwitch.enableTransmit(transmitterPin);
    //rcSwitch.setRepeatTransmit(6);

    setup_watchdog(9);
}

void loop()
{
    //if (f_wdt==1) {  // wait for timed out watchdog / flag is set when a watchdog timeout occurs
    //    f_wdt=0;       // reset flag
    //}

    if (counter > counterStart){
        counter = 0;
        sleeps++;
        sendMsg();
    }

    system_sleep();
}

// Watchdog Interrupt Service / is executed when watchdog timed out
ISR(WDT_vect) {
    counter++;
    f_wdt=1;  // set global flag
}