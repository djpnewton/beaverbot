/* Teensy RawHID example
 * http://www.pjrc.com/teensy/rawhid.html
 * Copyright (c) 2009 PJRC.COM, LLC
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above description, website URL and copyright notice and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "usb_rawhid.h"
#include "analog.h"
#include "kowhai/kowhai_protocol_server.h"

#define LED_ON      (PORTD |= (1<<6))
#define LED_OFF     (PORTD &= ~(1<<6))
#define LED_CONFIG  (DDRD |= (1<<6))
#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

//
// kowhai symbols
//

#include "symbols.h"

//
// kowhai tree descriptor
//

#define PORT_COUNT 6
#define ADC_COUNT 8

struct kowhai_node_t teensy_descriptor[] =
{
    { KOW_BRANCH_START,     SYM_TEENSY,         1,                0 },
    { KOW_UINT8,            SYM_LED,            1,                0 },
    { KOW_BRANCH_START,     SYM_GPIO,           PORT_COUNT,       0 },
    { KOW_UINT8,            SYM_DDR,            1,                0 },
    { KOW_UINT8,            SYM_PORT,           1,                0 },
    { KOW_UINT8,            SYM_PIN,            1,                0 },
    { KOW_BRANCH_END,       SYM_GPIO,           1,                0 },
    { KOW_BRANCH_START,     SYM_ADC,            1,                0 },
    { KOW_UINT8,            SYM_DIDR0,          1,                0 },
    { KOW_UINT8,            SYM_DIDR1,          1,                0 },
    { KOW_UINT16,           SYM_ADC,            ADC_COUNT,        0 },
    { KOW_BRANCH_END,       SYM_ADC,            1,                0 },
    { KOW_BRANCH_START,     SYM_TIMER2,         1,                0 },
    { KOW_UINT8,            SYM_TCCR2A,         1,                0 },
    { KOW_UINT8,            SYM_TCCR2B,         1,                0 },
    { KOW_UINT8,            SYM_OCR2A,          1,                0 },
    { KOW_UINT8,            SYM_OCR2B,          1,                0 },
    { KOW_BRANCH_END,       SYM_TIMER2,         1,                0 },
    { KOW_UINT8,            SYM_PROGRAM,        1,                0 },
    { KOW_BRANCH_END,       SYM_TEENSY,         1,                0 },
};

//
// kowhai tree structs
//

struct gpio_t
{
    uint8_t ddr;
    uint8_t port;
    uint8_t pin;
};

struct adc_t
{
    uint8_t _DIDR0;
    uint8_t _DIDR1;
    uint16_t adc[ADC_COUNT];
};

struct timer2_t
{
    uint8_t _TCCR2A;
    uint8_t _TCCR2B;
    uint8_t _OCR2A;
    uint8_t _OCR2B;
};

struct teensy_t
{
    uint8_t LED;
    struct gpio_t GPIO[PORT_COUNT];
    struct adc_t adc;
    struct timer2_t timer2;
    uint8_t program;
};

struct teensy_t teensy = {0};

#define WRITE_PORT(port_settings, _ddr, _port)  \
    if (port_settings.ddr != _ddr)              \
        _ddr = port_settings.ddr;               \
    if (port_settings.port != _port)            \
        _port = port_settings.port;

#define READ_PIN(port_settings, _pin)           \
    port_settings.pin = _pin;

#define WRITE_REG(value, _reg)                  \
    if (value != _reg)                          \
        _reg = value;

#define READ_ADC(digital_input_disable, adc_pin)\
    if (DIDR0 & digital_input_disable)          \
        teensy.adc.adc[adc_pin] = adc_read(adc_pin);

void node_written(void* param, struct kowhai_node_t* node)
{
    if (teensy.LED)
        LED_ON;
    else
        LED_OFF;
    WRITE_PORT(teensy.GPIO[0], DDRA, PORTA);
    WRITE_PORT(teensy.GPIO[1], DDRB, PORTB);
    WRITE_PORT(teensy.GPIO[2], DDRC, PORTC);
    WRITE_PORT(teensy.GPIO[3], DDRD, PORTD);
    WRITE_PORT(teensy.GPIO[4], DDRE, PORTE);
    WRITE_PORT(teensy.GPIO[5], DDRF, PORTF);
    WRITE_REG(teensy.adc._DIDR0, DIDR0);
    WRITE_REG(teensy.adc._DIDR1, DIDR1);
    WRITE_REG(teensy.timer2._TCCR2A, TCCR2A);
    WRITE_REG(teensy.timer2._TCCR2B, TCCR2B);
    WRITE_REG(teensy.timer2._OCR2A, OCR2A);
    WRITE_REG(teensy.timer2._OCR2B, OCR2B);
}

void server_buffer_send(void* param, void* buffer, size_t buffer_size)
{
    usb_rawhid_send(buffer, 50);
}

void step_program(void)
{
#define PROGRAM_NULL 0
#define PROGRAM_STAY_ON_TABLE 1
    static long long int saw_table_edge = 0;
    switch (teensy.program)
    {
        case PROGRAM_STAY_ON_TABLE:
            // turn on pwm
            DDRB = 16;
            DDRD = 2;
            DDRF = 27;
            TCCR2A = 163;
            TCCR2B = 163;
            // turn on adc
            DIDR0 = 255;
            DIDR1 = 0;
            // check adc value of floor sensor
            if (teensy.adc.adc[7] > 800 || teensy.adc.adc[6] > 800)
                // init backward routine
                saw_table_edge = 100000;
            if (saw_table_edge)
            {
                // turn backwards
                PORTF = 9; 
                OCR2A = 30;
                OCR2B = 10;
                // only go backwards for a wee while
                saw_table_edge--;
            }
            else
            {
                // go forwards
                PORTF = 18; 
                OCR2A = 50;
                OCR2B = 50;
            }
            break;
    }
}

int main(void)
{
    int r;
    uint8_t buffer[RAWHID_PACKET_SIZE];

    // set for 16 MHz clock
    CPU_PRESCALE(0);

    // turn off led
    LED_CONFIG;
    LED_OFF;

    // Configure timer 0 to generate a timer overflow interrupt every
    // 256*64 clock cycles, or 1024 Hz when using 16 MHz clock
    TCCR0A = 0x00;
    TCCR0B = ((1 << CS00) | (1 << CS01));   // Set up timer at Fcpu/64 
    TIMSK0 = (1<<TOIE0);                    // Enable timer 0 overflow interrupt

    // Initialize the USB, and then wait for the host to set configuration.
    // If the Teensy is powered without a PC connected to the USB port,
    // this will wait forever.
    usb_init();
    while (!usb_configured()) /* wait */ ;

    // Wait an extra second for the PC's operating system to load drivers
    // and do whatever it does to actually be ready for input
    _delay_ms(1000);


    // init kowhai server
    struct kowhai_node_t* tree_descriptors[] = {teensy_descriptor};
    size_t tree_descriptor_sizes[] = {sizeof(teensy_descriptor)};
    void* tree_data_buffers[] = {&teensy};
    struct kowhai_protocol_server_t server = {RAWHID_PACKET_SIZE, buffer, node_written, NULL, server_buffer_send, NULL, 1, tree_descriptors, tree_descriptor_sizes, tree_data_buffers};

    // init teensy tree
    teensy.program = PROGRAM_STAY_ON_TABLE;

    while (1) {
        // if received data, do something with it
        r = usb_rawhid_recv(buffer, 0);
        if (r > 0) {
            kowhai_server_process_packet(&server, buffer, RAWHID_PACKET_SIZE);
        }
        step_program();
    }
}

int16_t adc_read(uint8_t mux)
{
#define ADC_REF_POWER     (1<<REFS0)
#define ADC_REF_INTERNAL  ((1<<REFS1) | (1<<REFS0))
#define ADC_REF_EXTERNAL  (0)

// These prescaler values are for high speed mode, ADHSM = 1
#if F_CPU == 16000000L
#define ADC_PRESCALER ((1<<ADPS2) | (1<<ADPS1))
#elif F_CPU == 8000000L
#define ADC_PRESCALER ((1<<ADPS2) | (1<<ADPS0))
#elif F_CPU == 4000000L
#define ADC_PRESCALER ((1<<ADPS2))
#elif F_CPU == 2000000L
#define ADC_PRESCALER ((1<<ADPS1) | (1<<ADPS0))
#elif F_CPU == 1000000L
#define ADC_PRESCALER ((1<<ADPS1))
#else
#define ADC_PRESCALER ((1<<ADPS0))
#endif

// some avr-libc versions do not properly define ADHSM
#if defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__)
#if !defined(ADHSM)
#define ADHSM (7)
#endif
#endif
    
    static uint8_t aref = ADC_REF_POWER;

    uint8_t low;

    ADCSRA = (1<<ADEN) | ADC_PRESCALER; // enable ADC
    ADCSRB = (1<<ADHSM) | (mux & 0x20); // high speed mode
    ADMUX = aref | (mux & 0x1F);        // configure mux input
    ADCSRA = (1<<ADEN) | ADC_PRESCALER | (1<<ADSC); // start the conversion
    while (ADCSRA & (1<<ADSC)) ;        // wait for result
    low = ADCL;                         // must read LSB first
    return (ADCH << 8) | low;           // must read MSB only once!
}


ISR(TIMER0_OVF_vect)
{
    /* Timer 0 overflow */

    READ_PIN(teensy.GPIO[0], PINA);
    READ_PIN(teensy.GPIO[1], PINB);
    READ_PIN(teensy.GPIO[2], PINC);
    READ_PIN(teensy.GPIO[3], PIND);
    READ_PIN(teensy.GPIO[4], PINE);
    READ_PIN(teensy.GPIO[5], PINF);

    READ_ADC(ADC0D, 0);
    READ_ADC(ADC1D, 1);
    READ_ADC(ADC2D, 2);
    READ_ADC(ADC3D, 3);
    READ_ADC(ADC4D, 4);
    READ_ADC(ADC5D, 5);
    READ_ADC(ADC6D, 6);
    READ_ADC(ADC7D, 7);
    READ_ADC(ADC0D, 0);
}
