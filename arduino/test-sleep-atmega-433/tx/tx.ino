#include <RH_ASK.h>
#ifdef RH_HAVE_HARDWARE_SPI
#include <SPI.h> // Not actually used but needed to compile
#endif

// RH_ASK.h
//--------------
// Caution: the default internal clock speed on an ATTiny85 is 1MHz. You MUST set the internal clock speed to 8MHz.

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

//                 +-\/-+
// Ain0 (D5) PB5  1|    |8  Vcc
// Ain3 (D3) PB3  2|    |7  PB2 (D2) Ain1
// Ain2 (D4) PB4  3|    |6  PB1 (D1) pwm1
//           GND  4|    |5  PB0 (D0) pwm0
//                 +----+
//
const uint8_t pin_tx           = 2; // (D2), pin 7
const uint8_t pin_txPower      = 4; // pin 3
const uint8_t pin_soilMoisture = 1; // pin 6
const uint8_t pin_button       = 3; // pin 2

// TODO will be randomly generated and stored in EEPROM
const char sensorId = 170;

uint16_t cycles = 0;

const uint8_t openAirReading = 590; //calibration data 1
const uint8_t waterReading = 290;   //calibration data 2
uint8_t moistureLevel = 0;
uint8_t moisturePercentage = 0;

// Sleeps per cycle (s/c) - timer to reset 
// (  0 means  1 s/c - approx 8 sec)
// ( 63 means 64 s/c - approx 8.5 min)
// (899 means approx 2 hours)
// ...for maximum watchdog time (9=8sec)
uint16_t sleepCounterRequired = 0;
uint16_t sleepCounter = sleepCounterRequired;

SystemStatus ss;
RH_ASK rh433(6000, -1, pin_tx, pin_txPower);

void setup_watchdog() {
    // https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-2586-AVR-8-bit-Microcontroller-ATtiny25-ATtiny45-ATtiny85_Datasheet.pdf

    // MCUSR - MCU Status Register
    // -----
    // WDRF  - Watchdog System Reset Flag

    // WDTCR - Watchdog Timer Control Register
    // -----
    // WDIE  - Watchdog Interrupt Enable
    // WDCE  - Watchdog Change Enable
    // WDE   - Watchdog System Reset Enable
    // WDP   - Watchdog Timer Prescaler

    // Clear the WDRF in MCUSR, allows to set WDE
    MCUSR &= ~_BV(WDRF);

    // Setting WDCE in WDTCR allows updates for 4 clock cycles
    // Needed to change WDE or WDP
    WDTCR |= _BV(WDCE) | _BV(WDE);

    // Set new watchdog timeout value (WDP (Watchdog Timer Prescaler)) in WDTCR
    // Defines values for the WDT to timeout
    // 0=16ms, 1=32ms, 2=64ms, 3=128ms, 4=250ms, 5=500ms
    // 6=1sec, 7=2sec, 8=4sec, 9=8sec
    // (Selected hard-coded value is 9)
    WDTCR = (1<<WDP3) | (0<<WDP2) | (0<<WDP1) | (1<<WDP0);;

    // Set WDIE in WDTCR to allows interrupts
    WDTCR |= _BV(WDIE);
}

// system wakes up when watchdog is timed out
void system_sleep() {
    // Switch ADC off
    cbi(ADCSRA, ADEN);

    setup_watchdog();
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);

    // Enable pin change interrupts
    sbi(GIMSK, PCIE);
    // Enable PB3 pin interrupt
    sbi(PCMSK, PCINT3);

    // Enable sleep
    sleep_enable();
    // Enter sleep mode and wait for INT0
    sleep_mode();

    // ----- MCU is sleeping -----

    // Disable sleep after wakeup
    sleep_disable();

    // Diable PB3 pin interrupt
    cbi(PCMSK, PCINT3);

    // Switch ADC on
    sbi(ADCSRA, ADEN);
}

void sendMsg() {
    // Get VCC voltage (supercap)
    uint8_t capVoltage = ss.getVCC() / 100;

    // Get moisture level
    moistureLevel = analogRead(pin_soilMoisture);
    moisturePercentage = map(moistureLevel, openAirReading, waterReading, 0, 100);

    // Create message
    uint32_t message;
    message |= (uint32_t)sensorId   << 24; //     8 bits, 24 left
    message |= (uint32_t)cycles     << 13; //    11 bits, 13 left
    message |= (uint32_t)capVoltage <<  7; //     6 bits,  7 left
    message |= moisturePercentage & 0b1111111; // 7 bits,  0 left

    // Split message
    uint8_t messageArray[4];
    messageArray[0] = message >> 24;
    messageArray[1] = message >> 16;
    messageArray[2] = message >>  8;
    messageArray[3] = message;

    // Send message
    rh433.send((uint8_t *)messageArray, 4);
    rh433.waitPacketSent();
}

// Watchdog Interrupt Service - is executed when watchdog timed out
ISR(WDT_vect) {
    sleepCounter++;
}

// Pin Change Interrupt Service - is executed when pin change is detected
ISR(PCINT0_vect) {
    // Set the sleepCounter to the value required to send the message
    // This will cause the message will be send immediately
    sleepCounter = sleepCounterRequired + 1;
}

void setup() {
    pinMode(pin_soilMoisture, INPUT);
    pinMode(pin_button, INPUT);
    digitalWrite(pin_button, HIGH);

    // Enable interrupts so the WDT can wake us up
    sei();

    rh433.init();

    setup_watchdog();
}

void loop() {
    if (sleepCounter > sleepCounterRequired){
        sleepCounter = 0;
        cycles++;
        sendMsg();
    }

    system_sleep();
}
