#include "can_search.h"

#include <stdlib.h>
#include <util/delay.h>

volatile enum search_states_t g_current_state = ST_INIT;
enum search_mode_t g_search_mode = SM_PUSH;
cam_search_motor_set_t g_motor_set_callback;

#define BASE_DUTY_CYCLE 80
#define SPIN_DUTY_CYCLE 50
#define SPIN_SUPER_DUTY_CYCLE 250
#define SPIN_SUPER_TIMEOUT 120
#define REVERSE_TIMEOUT 1000
#define SPIN_TIMEOUT_MAX 1500
#define SPIN_TIMEOUT_MIN 400
#define SEARCH_TIMEOUT 5000
#define SCRAMBLE_TIMEOUT 500

volatile int g_reverse_time = -1;
volatile int g_spin_time = -1;
volatile int g_spin_timeout = SPIN_TIMEOUT_MIN;
volatile int g_search_time = -1;
volatile int g_scramble_time = -1;

void set_motors_from_can_pos(float x, float y)
{
    uint8_t value1 = BASE_DUTY_CYCLE;
    uint8_t value2 = BASE_DUTY_CYCLE;
    if (x < 0.5f)
        value2 = (uint8_t)(x / 0.5f * BASE_DUTY_CYCLE);
    if (x > 0.5f)
        value1 = (uint8_t)((x - 0.5f) / 0.5f * BASE_DUTY_CYCLE);
    g_motor_set_callback(1, value1, 1, value2);
}

void set_left_motor(void)
{
    g_motor_set_callback(0, 0, 1, BASE_DUTY_CYCLE);
}

void set_right_motor(void)
{
    g_motor_set_callback(1, BASE_DUTY_CYCLE, 0, 0);
}

void set_motors_forwards(void)
{
    g_motor_set_callback(1, BASE_DUTY_CYCLE, 1, BASE_DUTY_CYCLE);
}

void set_motors_forwards_left(void)
{
    g_motor_set_callback(1, BASE_DUTY_CYCLE * 3 / 4, 1, BASE_DUTY_CYCLE);
}

void set_motors_forwards_right(void)
{
    g_motor_set_callback(1, BASE_DUTY_CYCLE * 3 / 4, 1, BASE_DUTY_CYCLE / 2);
}

void set_motors_backwards(void)
{
    g_motor_set_callback(2, BASE_DUTY_CYCLE, 2, BASE_DUTY_CYCLE);
}

void set_motors_none(void)
{
    g_motor_set_callback(0, 0, 0, 0);
}

void set_motors_spin_right(void)
{
    g_motor_set_callback(2, SPIN_DUTY_CYCLE, 1, SPIN_DUTY_CYCLE);
}

void set_motors_spin_left(void)
{
    g_motor_set_callback(1, SPIN_DUTY_CYCLE, 2, SPIN_DUTY_CYCLE);
}

void set_motors_spin_super(void)
{
    g_motor_set_callback(2, SPIN_SUPER_DUTY_CYCLE, 1, SPIN_SUPER_DUTY_CYCLE);
}

void transition(enum search_states_t new_state)
{
    can_search_signal(SIG_EXIT, 0, 0);
    g_current_state = new_state;
    can_search_signal(SIG_ENTRY, 0, 0);
}

void init(enum state_signals_t signal, float p1, float p2)
{
}

void search(enum state_signals_t signal, float p1, float p2)
{
    switch (signal)
    {
        case SIG_ENTRY:
        {
            int r = rand();
            if (r < RAND_MAX / 3)
                set_motors_forwards();
            else if (r < 2 * RAND_MAX / 3)
                set_motors_forwards_left();
            else
                set_motors_forwards_right();
            g_search_time = 0;
            break;
        }
        case SIG_EXIT:
            g_search_time = -1;
            break;
        case SIG_CAN_SPOTTED:
            if (p1 > 0.35f && p1 < 0.65f && p2 > 0.60)
            {
                if (g_search_mode == SM_PUSH)
                    transition(ST_PUSH);
                else
                    transition(ST_SUPER_SPIN);
            }
            else
            {
                set_motors_from_can_pos(p1, p2);
                g_search_time = 0;
            }
            break;
        case SIG_FRONT_LEFT:
            if ((int)p1)
                transition(ST_REVERSE);
            break;
        case SIG_FRONT_RIGHT:
            if ((int)p1)
                transition(ST_REVERSE);
            break;
        case SIG_SEARCH_TIMEOUT:
            transition(ST_SEARCH_SCRAMBLE);
            break;
        default:
            break;
    }
}

void super_spin(enum state_signals_t signal, float p1, float p2)
{
     switch (signal)
    {
        case SIG_ENTRY:
            set_motors_spin_super();
            _delay_ms(SPIN_SUPER_TIMEOUT);
            transition(ST_SEARCH);
            break;
        default:
            break;
    }
}

void push(enum state_signals_t signal, float p1, float p2)
{
     switch (signal)
    {
        case SIG_ENTRY:
            set_motors_forwards();
            break;
        case SIG_FRONT_LEFT:
            if ((int)p1)
                transition(ST_PUSH_RIGHT);
            break;
        case SIG_FRONT_RIGHT:
            if ((int)p1)
                transition(ST_PUSH_LEFT);
            break;            
        default:
            break;
    }
}

void push_left(enum state_signals_t signal, float p1, float p2)
{
    switch (signal)
    {
        case SIG_ENTRY:
            set_motors_spin_right();
            break;
        case SIG_FRONT_RIGHT:
            if (!(int)p1)
                set_motors_forwards();
            else
                set_motors_spin_right();
            break;
        case SIG_FRONT_LEFT:
            if ((int)p1)
                transition(ST_REVERSE);
            break;
        default:
            break;
    }
}

void push_right(enum state_signals_t signal, float p1, float p2)
{
    switch (signal)
    {
        case SIG_ENTRY:
            set_motors_spin_left();
            break;
        case SIG_FRONT_LEFT:
            if (!(int)p1)
                set_motors_forwards();
            else
                set_motors_spin_left();
            break;
        case SIG_FRONT_RIGHT:
            if ((int)p1)
                transition(ST_REVERSE);
            break;
        default:
            break;
    }
}

void reverse(enum state_signals_t signal, float p1, float p2)
{
    switch (signal)
    {
        case SIG_ENTRY:
            g_reverse_time = 0;
            set_motors_backwards();
            break;
        case SIG_REVERSE_TIMEOUT:
            transition(ST_SPIN);
            break;
        case SIG_BACK_LEFT:
        case SIG_BACK_RIGHT:
            if ((int)p1)
                transition(ST_SEARCH);
            break;
        default:
            break;
    }
}

void spin(enum state_signals_t signal, float p1, float p2)
{
    switch (signal)
    {
        case SIG_ENTRY:
        {
            float spin_extra = (SPIN_TIMEOUT_MAX - SPIN_TIMEOUT_MIN) * (float)rand() / RAND_MAX;
            g_spin_timeout = SPIN_TIMEOUT_MIN + (int)spin_extra;
            g_spin_time = 0;
            set_motors_spin_right();
            break;
        }
        case SIG_SPIN_TIMEOUT:
            transition(ST_SEARCH);
            break;
        case SIG_CAN_SPOTTED:
            transition(ST_SEARCH);
            break;
        case SIG_BACK_LEFT:
        case SIG_BACK_RIGHT:
            if ((int)p1)
                transition(ST_SEARCH);
            break;
        default:
            break;
    }
}

void search_scramble(enum state_signals_t signal, float p1, float p2)
{
    switch (signal)
    {
        case SIG_ENTRY:
            g_scramble_time= 0;
            set_motors_spin_right();
            break;
        case SIG_SCRAMBLE_TIMEOUT:
            transition(ST_SEARCH);
            break;
        case SIG_CAN_SPOTTED:
            transition(ST_SEARCH);
            break;
        default:
            break;
    }
}

void can_search_init(enum search_mode_t mode, cam_search_motor_set_t motor_set_callback)
{
    g_motor_set_callback = motor_set_callback;
    g_current_state = ST_INIT;
    g_search_mode = mode;
    if (mode == SM_PUSH)
        transition(ST_PUSH);
    else
        transition(ST_SEARCH);
}

void can_search_signal(enum state_signals_t signal, float p1, float p2)
{
    switch (g_current_state)
    {
        case ST_INIT:
            init(signal, p1, p2);
        case ST_SEARCH:
            search(signal, p1, p2);
            return;
        case ST_SUPER_SPIN:
            super_spin(signal, p1, p2);
        case ST_PUSH:
            push(signal, p1, p2);
            return;
        case ST_PUSH_LEFT:
            push_left(signal, p1, p2);
            return;
        case ST_PUSH_RIGHT:
            push_right(signal, p1, p2);
            return;
        case ST_REVERSE:
            reverse(signal, p1, p2);
            return;
        case ST_SPIN:
            spin(signal, p1, p2);
            return;
        case ST_SEARCH_SCRAMBLE:
            search_scramble(signal, p1, p2);
            return;
    }
}

// we assume this is called about once per millisecond
void can_search_tick(void)
{
    if (g_reverse_time >= 0)
    {
        g_reverse_time++;
        if (g_reverse_time > REVERSE_TIMEOUT)
        {
            can_search_signal(SIG_REVERSE_TIMEOUT, 0, 0);
            g_reverse_time = -1;
        }
    }
    if (g_spin_time >= 0)
    {
        g_spin_time++;
        if (g_spin_time > g_spin_timeout)
        {
            can_search_signal(SIG_SPIN_TIMEOUT, 0, 0);
            g_spin_time = -1;
        }
    }
    if (g_search_time >= 0)
    {
        g_search_time++;
        if (g_search_time > SEARCH_TIMEOUT)
        {
            can_search_signal(SIG_SEARCH_TIMEOUT, 0, 0);
            g_search_time = -1;
        }
    }
    if (g_scramble_time >= 0)
    {
        g_scramble_time++;
        if (g_scramble_time > SCRAMBLE_TIMEOUT)
        {
            can_search_signal(SIG_SCRAMBLE_TIMEOUT, 0, 0);
            g_scramble_time = -1;
        }
    }
}
