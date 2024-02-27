#include <RH_ASK.h>
#ifdef RH_HAVE_HARDWARE_SPI
#include <SPI.h> // Not actually used but needed to compile
#endif

// RH_ASK.cpp
//--------------
// RH_ASK on ATtiny8x uses Timer 0 to generate interrupts 8 times per bit interval.
// Timer 0 is used by Arduino platform for millis()/micros() which is used by delay()
// Uncomment the define RH_ASK_ATTINY_USE_TIMER1 bellow, if you want to use Timer 1 instead of Timer 0 on ATtiny
// Timer 1 is also used by some other libraries, e.g. Servo. Alway check usage of Timer 1 before enabling this.
//  Should be moved to header file
//#define RH_ASK_ATTINY_USE_TIMER1

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

RH_ASK driver(6000, -1, transmitterPin, -1);

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
    // Enable power to the 433 transmitter
    digitalWrite(transmitterPowerPin, HIGH);

    // Get VCC voltage (supercap)
    uint8_t capVoltage = ss.getVCC() / 100;

    // Get moisture level
    moistureLevel = analogRead(soilMoisturePin);
    moisturePercentage = map(moistureLevel, openAirReading, waterReading, 0, 100);

    // Create message
    uint32_t message;
    message |= (uint32_t)sensorId   << 24; //     8 bits, 24 left
    message |= (uint32_t)sleeps     << 13; //    11 bits, 13 left
    message |= (uint32_t)capVoltage <<  7; //     6 bits,  7 left
    message |= moisturePercentage & 0b1111111; // 7 bits,  0 left

    // Split message
    uint8_t messageArray[4];
    messageArray[0] = message >> 24;
    messageArray[1] = message >> 16;
    messageArray[2] = message >>  8;
    messageArray[3] = message;

    driver.send((uint8_t *)messageArray, 4);
    driver.waitPacketSent();

    // Disable power to the 433 transmitter
    digitalWrite(transmitterPowerPin, LOW);
}

void setup()
{
    pinMode(heartPin, OUTPUT);
    pinMode(transmitterPowerPin, OUTPUT);
    pinMode(soilMoisturePin, INPUT);

    digitalWrite(heartPin, LOW);
    digitalWrite(transmitterPowerPin, LOW);

    driver.init();

    for (int i = 0; i < 10; i++) {
      digitalWrite(transmitterPowerPin, HIGH);
      delay(10);
      digitalWrite(transmitterPowerPin, LOW);
      delay(10);
    }

    //setup_watchdog(9);
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

    for (int i = 0; i < 4; i++) {
      digitalWrite(transmitterPowerPin, HIGH);
      delay(10);
      digitalWrite(transmitterPowerPin, LOW);
      delay(10);
    }
}

// Watchdog Interrupt Service / is executed when watchdog timed out
ISR(WDT_vect) {
    counter++;
    f_wdt=1;  // set global flag
}