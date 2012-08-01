#include "can_search.h"

volatile enum search_states_t g_current_state = ST_INIT;
cam_search_motor_set_t g_motor_set_callback;

#define REVERSE_TIMEOUT 1000
#define SPIN_TIMEOUT 750
#define SEARCH_TIMEOUT 5000
#define SCRAMBLE_TIMEOUT 500

volatile int g_reverse_time = -1;
volatile int g_spin_time = -1;
volatile int g_search_time = -1;
volatile int g_scramble_time = -1;

#define BASE_POWER 40
void set_motors_from_can_pos(float x, float y)
{
    uint8_t value1 = BASE_POWER;
    uint8_t value2 = BASE_POWER;
    if (x < 0.5f)
        value2 = (uint8_t)(x / 0.5f * BASE_POWER);
    if (x > 0.5f)
        value1 = (uint8_t)((x - 0.5f) / 0.5f * BASE_POWER);
    g_motor_set_callback(1, value1, 1, value2);
}

void set_left_motor(void)
{
    g_motor_set_callback(0, 0, 1, BASE_POWER);
}

void set_right_motor(void)
{
    g_motor_set_callback(1, BASE_POWER, 0, 0);
}

void set_motors_forwards(void)
{
    g_motor_set_callback(1, BASE_POWER, 1, BASE_POWER);
}

void set_motors_backwards(void)
{
    g_motor_set_callback(2, BASE_POWER, 2, BASE_POWER);
}

void set_motors_spin(void)
{
    g_motor_set_callback(1, BASE_POWER, 2, BASE_POWER);
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
            set_motors_forwards();
            g_search_time = 0;
            break;
        case SIG_EXIT:
            g_search_time = -1;
            break;
        case SIG_CAN_SPOTTED:
            set_motors_from_can_pos(p1, p2);
            g_search_time = 0;
            break;
        case SIG_FRONT_LEFT:
            if ((int)p1)
                transition(ST_PUSH_RIGHT);
            break;
        case SIG_FRONT_RIGHT:
            if ((int)p1)
                transition(ST_PUSH_LEFT);
            break;
        case SIG_SEARCH_TIMEOUT:
            transition(ST_SEARCH_SCRAMBLE);
            break;
        default:
            break;
    }
}

void push(enum state_signals_t signal, float p1, float p2)
{
    //TODO
}

void push_left(enum state_signals_t signal, float p1, float p2)
{
    switch (signal)
    {
        case SIG_ENTRY:
            set_left_motor();
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
            set_right_motor();
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
            g_spin_time = 0;
            set_motors_spin();
            break;
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
            set_motors_spin();
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

void can_search_init(cam_search_motor_set_t motor_set_callback)
{
    g_motor_set_callback = motor_set_callback;
    g_current_state = ST_INIT;
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
        if (g_spin_time > SPIN_TIMEOUT)
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
