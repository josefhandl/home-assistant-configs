
#include <RH_ASK.h>
//#ifdef RH_HAVE_HARDWARE_SPI
//#include <SPI.h> // Not actually used but needed to compile
//#endif
#include <Adafruit_BMP280.h>


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

//#include "SystemStatus.h"

// these define cbi and sbi, for as far they are not known yet
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

// ATtiny1614
//----------------
// 14-pin SOIC, 8-bit, 16kB Flash, 2kB SRAM, 256B EEPROM, 20MHz
//
//                 +----+
//           VDD  1|O   |14  GND
//   SS AIN4 PA4  2|    |13  PA3 AIN3 SCK EXTCLK
// VREF AIN5 PA5  3|    |12  PA2 AIN2 MISO EVOUTA
//      AIN6 PA6  4|    |11  PA1 AIN1 MOSI
//      AIN7 PA7  5|    |10  PA0 AIN0 RESET UPDI
// RxD TOSC1 PB3  6|    | 9  PB0 AIN11 SCL XDIR
// TxD TOSC2 PB2  7|    | 8  PB1 AIN10 SDA XCK
//                 +----+

const uint8_t pin_tx           = PIN_PB2;
const uint8_t pin_txPower      = PIN_PB3;
const uint8_t pin_led          = PIN_PA6;
//const uint8_t pin_soilMoisture = PIN_PA7;
const uint8_t pin_button       = PIN_PA3;

const uint8_t pin_unused0      = PIN_PA0;
const uint8_t pin_unused3      = PIN_PA4;
const uint8_t pin_unused4      = PIN_PA5;
const uint8_t pin_unused7      = PIN_PA7;
const uint8_t pin_unused2      = PIN_PA2;
const uint8_t pin_unused1      = PIN_PA1;
const uint8_t pin_unused5      = PIN_PB0;
const uint8_t pin_unused6      = PIN_PB1;

// TODO will be randomly generated and stored in EEPROM
const char sensorId = 170;

uint16_t cycles = 0;

uint8_t moistureLevel = 0;
uint8_t moisturePercentage = 0;

// Sleeps per cycle (s/c) - timer to reset 
// (  0 means  1 s/c - approx 8 sec)
// ( 63 means 64 s/c - approx 8.5 min)
// (899 means approx 2 hours)
// ...for maximum watchdog time (9=8sec)
uint16_t sleepCounterRequired = 0;
uint16_t sleepCounter = sleepCounterRequired;

//SystemStatus ss;

// Preambule lenght (this device)
// 6000 attiny1614 4 MHz - 93.23 Hz, 10.73 ms
// 5500 attiny1614 4 MHz - 91.57 Hz, 10.95 ms
RH_ASK rh433(5500, -1, pin_tx, pin_txPower);

Adafruit_BMP280 bmp280;

void setup_pit() {
    // Set the watchdog timer to trigger an interrupt after 8 seconds
    //_PROTECTED_WRITE(WDT.CTRLA, WDT_PERIOD_8KCLK_gc);  // Set 8s timeout

    // Enable watchdog interrupt mode (WDT will trigger an interrupt instead of a reset)
    //_PROTECTED_WRITE(WDT.CTRLB, WDT_WINDOW_OFF_gc);    // No window mode, just standard timeout
    //WDT.INTCTRL = WDT_INTEN_bm;  // Enable watchdog interrupt


    // Configure the RTC Counter
    // Use the internal 1.024 kHz oscillator for the RTC
    RTC.CLKSEL = RTC_CLKSEL_INT1K_gc;
    // Enable the PIT with a 1-second interval (1024 cycles of 1.024 kHz clock)
    //RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc | RTC_PITEN_bm;  // Enable PIT
    RTC.PITCTRLA = RTC_PERIOD_CYC8192_gc | RTC_PITEN_bm;  // Enable PIT
    // Enable the PIT interrupt
    RTC.PITINTCTRL = RTC_PI_bm;
}

// system wakes up when watchdog is timed out
void system_sleep() {
    // Switch ADC off
    //cbi(ADCSRA, ADEN);
    //ADC0.CTRLA &= ~ADC_ENABLE_bm; // Disable ADC to save power

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);

    // Enable pin change interrupts
    //sbi(GIMSK, PCIE);
    // Enable PB3 pin interrupt
    //sbi(PCMSK, PCINT3);

    // Enable pin change interrupt on PA3
    //PORTA.PIN3CTRL = PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc; // ISR(PORTA_PORT_vect) definition causes conflict with RadioHead
    attachInterrupt(digitalPinToInterrupt(PIN_PA3), isr_pin, CHANGE);
    // PORT_ISC_FALLING_gc doesn't work for PA3 for some reason
    // https://forum.arduino.cc/t/tinyavr-series-1-e-g-attiny1614-wake-up-from-interrupt/912065

    // Enable sleep
    sleep_enable();
    // Enter sleep mode and wait for INT0
    sleep_cpu();

    // ----- MCU is sleeping -----

    // Disable sleep after wakeup
    sleep_disable();

    // Diable PB3 pin interrupt
    //cbi(PCMSK, PCINT3);

    // Switch ADC on
    //sbi(ADCSRA, ADEN);
    //ADC0.CTRLA |= ADC_ENABLE_bm;  // Enable ADC
}

void sendMsg() {
    // Get VCC voltage (supercap)
    getVccSetup();
    uint8_t capVoltage = getVcc();
    getVccReset();

    // Get moisture level
    bmp280.takeForcedMeasurement(); // Init reading the data from sensor in sleep mode
    moisturePercentage = (int8_t)(bmp280.readTemperature());

    // Create message
    uint32_t message = 0;
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

// http://www.technoblogy.com/show?3K82
void getVccSetup () {
    VREF.CTRLA = VREF_ADC0REFSEL_1V1_gc;
    ADC0.CTRLC = ADC_REFSEL_VDDREF_gc | ADC_PRESC_DIV256_gc; // 78kHz clock
    ADC0.MUXPOS = ADC_MUXPOS_INTREF_gc;                  // Measure from INTREF (INTREF as input)
    ADC0.CTRLA = ADC_ENABLE_bm;                          // Single, 10-bit
}

uint8_t getVcc () {
    ADC0.COMMAND = ADC_STCONV_bm;                        // Start conversion
    while (ADC0.COMMAND & ADC_STCONV_bm);                // Wait for completion
    uint16_t adc_reading = ADC0.RES;                     // ADC conversion result
    uint16_t voltage = 11264/adc_reading;
    return (uint8_t)voltage;
}

uint8_t getVccReset() {
    ADC0.CTRLA &= ~ADC_ENABLE_bm;  // Disable ADC
    ADC0.MUXPOS = ADC_MUXPOS_GND_gc;  // Set input to GND
}


// Watchdog Interrupt Service - is executed when watchdog timed out
ISR(RTC_PIT_vect) {
    // Clear the overflow flag
    RTC.PITINTFLAGS = RTC_PI_bm;  // Clear the interrupt flag

    sleepCounter++;
}

// Pin Change Interrupt Service - is executed when pin change is detected
//ISR(PORTB_PORT_vect) { // This causes conflict with RadioHead
void isr_pin() {
    // Clear the interrupt flag for PA3
    //PORTA.INTFLAGS = PIN3_bm;  // Clear PA3 interrupt flag
    sleep_disable();

    // Set the sleepCounter to the value required to send the message
    // This will cause the message will be send immediately
    sleepCounter = sleepCounterRequired + 1;
}


void setup() {
    pinMode(pin_unused0, INPUT_PULLUP);
    pinMode(pin_unused1, INPUT_PULLUP);
    pinMode(pin_unused2, INPUT_PULLUP);
    pinMode(pin_unused3, INPUT_PULLUP);
    pinMode(pin_unused4, INPUT_PULLUP);
    pinMode(pin_unused5, INPUT_PULLUP);
    pinMode(pin_unused6, INPUT_PULLUP);
    pinMode(pin_unused7, INPUT_PULLUP);

    //pinMode(pin_soilMoisture, INPUT);
    pinMode(pin_led, OUTPUT);
    digitalWrite(pin_led, HIGH);
    pinMode(pin_button, INPUT_PULLUP);
    //digitalWrite(pin_button, HIGH);

    // Enable interrupts so the WDT can wake us up
    sei();

    if (!rh433.init()) {
        while (true) {
            digitalWrite(pin_led, LOW);
            delay(100);
            digitalWrite(pin_led, HIGH);
            delay(100);
        }
    }

    bool status;

    // default settings
    // (you can also pass in a Wire library object like &Wire2)
    status = bmp280.begin(0x76);
    if (!status) {
        while (1);
    }

    // Set Forced mode to put sensor to sleep after measurement
    bmp280.setSampling(Adafruit_BMP280::MODE_FORCED,
                Adafruit_BMP280::SAMPLING_X1, // Temperature sampling set to 1
                Adafruit_BMP280::SAMPLING_X1, // Pressure sampling set to 1
                Adafruit_BMP280::FILTER_OFF   // Filter off - immediate 100% step response
                );

    setup_pit();
}

void loop() {
    if (sleepCounter > sleepCounterRequired){
        sleepCounter = 0;
        cycles++;
        sendMsg();
    }

    system_sleep();
}
