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
#include "can_search.h"

#define LED_ON      (PORTD |= (1<<6))
#define LED_OFF     (PORTD &= ~(1<<6))
#define LED_CONFIG  (DDRD |= (1<<6))
#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

//
// kowhai symbols
//

#include "symbols.h"

//
// kowhai tree descriptor
//

#define PORT_COUNT 6
#define ADC_COUNT 8
#define TABLE_SENSOR_COUNT 4
#define MOTOR_COUNT 2

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
    { KOW_BRANCH_START,     SYM_TABLESENSOR,    TABLE_SENSOR_COUNT,0 },
    { KOW_UINT16,           SYM_TRIGGER,        1,                0 },
    { KOW_UINT16,           SYM_VALUE,          1,                0 },
    { KOW_UINT8,            SYM_TRIGGERED,      1,                0 },
    { KOW_BRANCH_END,       SYM_TABLESENSOR,    0,                0 },
    { KOW_BRANCH_END,       SYM_TEENSY,         1,                0 },
};

struct kowhai_node_t table_sensor_event_descriptor[] =
{
    { KOW_BRANCH_START,     SYM_TABLESENSOREVENT,1,               0 },
    { KOW_UINT8,            SYM_SENSOR,         1,                0 },
    { KOW_UINT8,            SYM_TRIGGERED,      1,                0 },
    { KOW_UINT16,           SYM_VALUE,          1,                0 },
    { KOW_BRANCH_END,       SYM_TABLESENSOREVENT,1,               0 },
};

struct kowhai_node_t motor_set_descriptor[] =
{
    { KOW_BRANCH_START,     SYM_MOTORSET,       1,                0 },
    { KOW_BRANCH_START,     SYM_MOTOR,          MOTOR_COUNT,      0 },
    { KOW_UINT8,            SYM_DIRECTION,      1,                0 },
    { KOW_UINT8,            SYM_VALUE,          1,                0 },
    { KOW_BRANCH_END,       SYM_MOTOR,          1,                0 },
    { KOW_BRANCH_END,       SYM_MOTORSET,       1,                0 },
};

struct kowhai_node_t beep_descriptor[] =
{
    { KOW_BRANCH_START,     SYM_BEEP,           1,                0 },
    { KOW_UINT16,           SYM_FREQUENCY,      1,                0 },
    { KOW_UINT16,           SYM_DURATION,       1,                0 },
    { KOW_BRANCH_END,       SYM_BEEP,           1,                0 },
};

struct kowhai_node_t guidance_descriptor[] =
{
    { KOW_BRANCH_START,     SYM_GUIDANCE,       1,                0 },
    { KOW_UINT16,           SYM_X,              1,                0 },
    { KOW_UINT16,           SYM_Y,              1,                0 },
    { KOW_UINT16,           SYM_WINDOWWIDTH,    1,                0 },
    { KOW_UINT16,           SYM_WINDOWHEIGHT,   1,                0 },
    { KOW_BRANCH_END,       SYM_GUIDANCE,       1,                0 },
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

struct table_sensor_t
{
    uint16_t trigger;
    uint16_t value;
    uint8_t triggered;
};

struct teensy_t
{
    uint8_t LED;
    struct gpio_t GPIO[PORT_COUNT];
    struct adc_t adc;
    struct timer2_t timer2;
    uint8_t program;
    struct table_sensor_t sensors[TABLE_SENSOR_COUNT];
};

struct table_sensor_event_t
{
    uint8_t sensor;
    uint8_t triggered;
    uint16_t value;
};

struct motor_t
{
    uint8_t direction;
    uint8_t value;
};

struct motor_set_t
{
    struct motor_t motor[MOTOR_COUNT];
};

struct beep_t
{
    uint16_t freq;
    uint16_t duration;
};

struct guidance_t
{
    uint16_t x, y;
    uint16_t window_width, window_height;
};

struct teensy_t teensy = {0};
struct motor_set_t motor_set = {};
struct beep_t beep = {0};
struct guidance_t guidance = {0};

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

void node_pre_write(pkowhai_protocol_server_t server, void* param, uint16_t tree_id, struct kowhai_node_t* node, int offset)
{
}

void node_post_write(pkowhai_protocol_server_t server, void* param, uint16_t tree_id, struct kowhai_node_t* node, int offset, int size)
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

void server_buffer_send(pkowhai_protocol_server_t server, void* param, void* buffer, size_t buffer_size)
{
    usb_rawhid_send(buffer, 50);
}

void motor_set_(struct motor_set_t* ms);

void do_beep(struct beep_t* beep)
{
    cli(); // disable interrupts

    DDRC = 3;
    int duration = beep->duration;
    while (duration > 0)
    {
        if (duration % beep->freq == 0)
            PORTC ^= 1;
        duration--;
    }

    sei(); // enable interrupts
}

enum program_t
{
    PROGRAM_NULL,
    PROGRAM_CALIBRATE_TABLE_SENSORS,
    PROGRAM_REPORT_SENSORS,
    PROGRAM_REPORT_SENSORS_WITH_BEEP,
    PROGRAM_STAY_ON_TABLE,
    PROGRAM_CAN_SEARCH_INIT,
    PROGRAM_BOOTLOADER,


    PROGRAM_CAN_SEARCH = 100,
};

int function_called(pkowhai_protocol_server_t server, void* param, uint16_t function_id)
{
    switch (function_id)
    {
        case SYM_MOTORSET:
            motor_set_(&motor_set);
            return 1;
        case SYM_BEEP:
            do_beep(&beep);
            return 1;
        case SYM_GUIDANCE:
            if (teensy.program == PROGRAM_CAN_SEARCH)
            {
                if (guidance.window_width != 0)
                    can_search_signal(SIG_CAN_SPOTTED,
                            guidance.x / (float)guidance.window_width,
                            guidance.y / (float)guidance.window_height);
            }
            return 1;
    }
    return 0;
}

void send_sensor_event(struct kowhai_protocol_server_t* server, struct table_sensor_event_t* event)
{
    kowhai_server_process_event(server, SYM_TABLESENSOREVENT, event, sizeof(struct table_sensor_event_t));
}

void motor_set__(uint8_t direction1, uint8_t value1, uint8_t direction2, uint8_t value2)
{
    struct motor_set_t ms;
    ms.motor[0].direction = direction1;
    ms.motor[0].value = value1;
    ms.motor[1].direction = direction2;
    ms.motor[1].value = value2;
    motor_set_(&ms);
}

void motor_set_(struct motor_set_t* ms)
{
    unsigned char portf = 0;
    if (ms->motor[0].direction == 1)
        portf |= 2;
    else if (ms->motor[0].direction == 2)
        portf |= 1;
    if (ms->motor[1].direction == 1)
        portf |= 8;
    else if (ms->motor[1].direction == 2)
        portf |= 4;
    PORTF = portf;
    OCR2A = ms->motor[0].value;
    OCR2B = ms->motor[1].value;
}

void stay_on_table(void)
{
    static long long int saw_table_edge_front = 0;
    static long long int saw_table_edge_rear = 0;
    // check adc value of floor sensor
    if (teensy.sensors[0].value > teensy.sensors[0].trigger ||
        teensy.sensors[1].value > teensy.sensors[1].trigger)
        // init backward routine
        saw_table_edge_front = 50000;
    if (teensy.sensors[2].value > teensy.sensors[2].trigger)
        // init oh-no! routine
        saw_table_edge_rear = 30000;
    if (saw_table_edge_front)
    {
        // turn backwards
        PORTF = 5; 
        OCR2A = 60;
        OCR2B = 20;
        // only go backwards for a wee while
        saw_table_edge_front--;
    }
    else
    {
        // go forwards
        PORTF = 10; 
        OCR2A = 100;
        OCR2B = 100;
    }
    if (saw_table_edge_rear)
    {
        // go nowhere
        PORTF = 0; 
        OCR2A = 0;
        OCR2B = 0;
        // only go nowhere for a wee while
        saw_table_edge_rear--;
    }
}

#define ADC_MAX 1024
#define ADC_HIST 10 // hysteresis
#define TRIGGER_RATIO (0.75f)
void calibrate_sensors(void)
{
    teensy.sensors[0].trigger = teensy.sensors[0].value + (ADC_MAX - teensy.sensors[0].value) * TRIGGER_RATIO;
    teensy.sensors[1].trigger = teensy.sensors[1].value + (ADC_MAX - teensy.sensors[1].value) * TRIGGER_RATIO;
    teensy.sensors[2].trigger = teensy.sensors[2].value + (ADC_MAX - teensy.sensors[2].value) * TRIGGER_RATIO;
    teensy.sensors[3].trigger = teensy.sensors[3].value + (ADC_MAX - teensy.sensors[3].value) * TRIGGER_RATIO;
}
int check_sensors(void)
{
    int i;
    for (i = 0; i < TABLE_SENSOR_COUNT; i++)
    {
        if (teensy.sensors[i].triggered)
        {
            if (teensy.sensors[i].value < teensy.sensors[i].trigger - ADC_HIST)
            {
                teensy.sensors[i].triggered = 0;
                return i;
            }
        }
        else
        {
            if (teensy.sensors[i].value > teensy.sensors[i].trigger + 50)
            {
                teensy.sensors[i].triggered = 1;
                return i;
            }
        }
    }
    return -1;
}

void bootloader(void)
{
    cli();
    // disable watchdog, if enabled
    // disable all peripherals
    UDCON = 1;
    USBCON = (1<<FRZCLK);  // disable USB
    UCSR1B = 0;
    _delay_ms(5);
#if defined(__AVR_AT90USB162__)                // Teensy 1.0
    EIMSK = 0; PCICR = 0; SPCR = 0; ACSR = 0; EECR = 0;
    TIMSK0 = 0; TIMSK1 = 0; UCSR1B = 0;
    DDRB = 0; DDRC = 0; DDRD = 0;
    PORTB = 0; PORTC = 0; PORTD = 0;
    asm volatile("jmp 0x3E00");
#elif defined(__AVR_ATmega32U4__)              // Teensy 2.0
    EIMSK = 0; PCICR = 0; SPCR = 0; ACSR = 0; EECR = 0; ADCSRA = 0;
    TIMSK0 = 0; TIMSK1 = 0; TIMSK3 = 0; TIMSK4 = 0; UCSR1B = 0; TWCR = 0;
    DDRB = 0; DDRC = 0; DDRD = 0; DDRE = 0; DDRF = 0; TWCR = 0;
    PORTB = 0; PORTC = 0; PORTD = 0; PORTE = 0; PORTF = 0;
    asm volatile("jmp 0x7E00");
#elif defined(__AVR_AT90USB646__)              // Teensy++ 1.0
    EIMSK = 0; PCICR = 0; SPCR = 0; ACSR = 0; EECR = 0; ADCSRA = 0;
    TIMSK0 = 0; TIMSK1 = 0; TIMSK2 = 0; TIMSK3 = 0; UCSR1B = 0; TWCR = 0;
    DDRA = 0; DDRB = 0; DDRC = 0; DDRD = 0; DDRE = 0; DDRF = 0;
    PORTA = 0; PORTB = 0; PORTC = 0; PORTD = 0; PORTE = 0; PORTF = 0;
    asm volatile("jmp 0xFC00");
#elif defined(__AVR_AT90USB1286__)             // Teensy++ 2.0
    EIMSK = 0; PCICR = 0; SPCR = 0; ACSR = 0; EECR = 0; ADCSRA = 0;
    TIMSK0 = 0; TIMSK1 = 0; TIMSK2 = 0; TIMSK3 = 0; UCSR1B = 0; TWCR = 0;
    DDRA = 0; DDRB = 0; DDRC = 0; DDRD = 0; DDRE = 0; DDRF = 0;
    PORTA = 0; PORTB = 0; PORTC = 0; PORTD = 0; PORTE = 0; PORTF = 0;
    asm volatile("jmp 0x1FC00");
#endif 
}

void step_program(struct kowhai_protocol_server_t* server)
{
    // get table sensor values
    teensy.sensors[0].value = teensy.adc.adc[7];
    teensy.sensors[1].value = teensy.adc.adc[6];
    teensy.sensors[2].value = teensy.adc.adc[5];
    teensy.sensors[3].value = teensy.adc.adc[4];
    // turn on pwm
    DDRB = 16;
    DDRD = 2;
    DDRF = 15;
    TCCR2A = 163;
    TCCR2B = 163;
    // turn on adc
    DIDR0 = 255;
    DIDR1 = 0;
    switch (teensy.program)
    {
        case PROGRAM_CALIBRATE_TABLE_SENSORS:
            calibrate_sensors();
            teensy.program = PROGRAM_NULL;
            break;
        case PROGRAM_REPORT_SENSORS:
        case PROGRAM_REPORT_SENSORS_WITH_BEEP:
        {
            // send table sensor events
            int i = check_sensors();
            if (i != -1)
            {
                struct table_sensor_event_t event = { i, teensy.sensors[i].triggered, teensy.sensors[i].value };
                send_sensor_event(server, &event);
                if (teensy.program == PROGRAM_REPORT_SENSORS_WITH_BEEP)
                {
                    struct beep_t beep = { 10 + 10 * i, 1000 };
                    do_beep(&beep);
                    if (teensy.sensors[i].triggered)
                    {
                        struct beep_t wait = { 0, 500 };
                        do_beep(&wait);
                        do_beep(&beep);
                    }
                }
            }
            break;
        }
        case PROGRAM_STAY_ON_TABLE:
            stay_on_table();
            break;
        case PROGRAM_CAN_SEARCH_INIT:
            calibrate_sensors();
            can_search_init(motor_set__);
            teensy.program = PROGRAM_CAN_SEARCH;
            break;
        case PROGRAM_CAN_SEARCH:
        {
            int i = check_sensors();
            if (i != -1)
            {
                switch (i)
                {
                    case 0:
                        can_search_signal(SIG_FRONT_LEFT, teensy.sensors[i].triggered, 0);
                        break;
                    case 1:
                        can_search_signal(SIG_FRONT_RIGHT, teensy.sensors[i].triggered, 0);
                        break;
                    case 2:
                        can_search_signal(SIG_BACK_LEFT, teensy.sensors[i].triggered, 0);
                        break;
                    case 3:
                        can_search_signal(SIG_BACK_RIGHT, teensy.sensors[i].triggered, 0);
                        break;
                }
            }
            break;
        }
        case PROGRAM_BOOTLOADER:
            bootloader();
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

    // Initialize the USB
    usb_init();
    // Wait for the host to set configuration.
    // If the Teensy is powered without a PC connected to the USB port,
    // this will wait forever.
    //while (!usb_configured()) /* wait */ ;

    // Wait an extra second for the PC's operating system to load drivers
    // and do whatever it does to actually be ready for input
    _delay_ms(1000);


    // init kowhai server
    uint16_t tree_list[] = {SYM_TEENSY, SYM_TABLESENSOREVENT, SYM_MOTORSET, SYM_BEEP, SYM_GUIDANCE};
    struct kowhai_node_t* tree_descriptors[] = {teensy_descriptor, table_sensor_event_descriptor, motor_set_descriptor, beep_descriptor, guidance_descriptor};
    size_t tree_descriptor_sizes[COUNT_OF(tree_descriptors)];
    void* tree_data_buffers[] = {&teensy, NULL, &motor_set, &beep, &guidance};
    uint16_t function_list[] = {SYM_MOTORSET, SYM_BEEP, SYM_GUIDANCE};
    struct kowhai_protocol_function_details_t function_list_details[] = {
        {SYM_MOTORSET, KOW_UNDEFINED_SYMBOL},
        {SYM_BEEP, KOW_UNDEFINED_SYMBOL},
        {SYM_GUIDANCE, KOW_UNDEFINED_SYMBOL},
    };
    kowhai_server_init_tree_descriptor_sizes(tree_descriptors, tree_descriptor_sizes, COUNT_OF(tree_descriptors));
    struct kowhai_protocol_server_t server;
    kowhai_server_init(&server,
            RAWHID_PACKET_SIZE,
            buffer,
            node_pre_write,
            node_post_write,
            NULL,
            server_buffer_send,
            NULL,
            COUNT_OF(tree_list),
            tree_list,
            tree_descriptors,
            tree_descriptor_sizes,
            tree_data_buffers,
            COUNT_OF(function_list),
            function_list,
            function_list_details,
            function_called,
            NULL,
            COUNT_OF(symbols),
            symbols);

    // init teensy tree
    teensy.program = PROGRAM_NULL;

    while (1) {
        // if received data, do something with it
        r = usb_rawhid_recv(buffer, 0);
        if (r > 0) {
            kowhai_server_process_packet(&server, buffer, RAWHID_PACKET_SIZE);
        }
        step_program(&server);
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

    if (teensy.program == PROGRAM_CAN_SEARCH)
        can_search_tick();
}
