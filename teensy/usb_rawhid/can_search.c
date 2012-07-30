#include "can_search.h"

static enum search_states_t g_current_state = ST_INIT;
cam_search_motor_set_t g_motor_set_callback;

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

void set_left_motor()
{
    g_motor_set_callback(0, 0, 1, BASE_POWER);
}

void set_right_motor()
{
    g_motor_set_callback(1, BASE_POWER, 0, 0);
}

void set_motors_backwards()
{
    g_motor_set_callback(2, BASE_POWER, 2, BASE_POWER);
}

void init(enum state_signals_t signal, float p1, float p2)
{
    g_current_state = ST_SEARCH;
}

void search(enum state_signals_t signal, float p1, float p2)
{
    switch (signal)
    {
        case SIG_INIT:
            g_motor_set_callback(1, 25, 1, 25);
            break;
        case SIG_CAN_SPOTTED:
            set_motors_from_can_pos(p1, p2);
            g_current_state = ST_APPROACH;
            break;
        case SIG_FRONT_LEFT:
            if ((int)p1)
            {
                set_right_motor();
            }
            break;
        case SIG_FRONT_RIGHT:
            if ((int)p1)
                set_left_motor();
            break;
        default:
            break;
    }
}

void approach(enum state_signals_t signal, float p1, float p2)
{
    switch (signal)
    {
        case SIG_CAN_SPOTTED:
            set_motors_from_can_pos(p1, p2);

            //TODO
            //if (something)
            //      g_current_state = ST_PUSH;
            break;
        case SIG_FRONT_LEFT:
            if ((int)p1)
            {
                set_right_motor();
                g_current_state = ST_PUSH_RIGHT;
            }
            break;
        case SIG_FRONT_RIGHT:
            if ((int)p1)
            {
                set_left_motor();
                g_current_state = ST_PUSH_LEFT;
            }
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
        case SIG_FRONT_RIGHT:
            if ((int)p1)
            {
                set_motors_backwards();
                g_current_state = ST_REVERSE_UTURN;
            }
            break;
        default:
            break;
    }
}

void push_right(enum state_signals_t signal, float p1, float p2)
{
    switch (signal)
    {
        case SIG_FRONT_LEFT:
            if ((int)p1)
            {
                set_motors_backwards();
                g_current_state = ST_REVERSE_UTURN;
            }
            break;
        default:
            break;
    }
}

void reverse_uturn(enum state_signals_t signal, float p1, float p2)
{
    //TODO
}

void search_scramble(enum state_signals_t signal, float p1, float p2)
{
    //TODO
}

void can_search_init(cam_search_motor_set_t motor_set_callback)
{
    g_motor_set_callback = motor_set_callback;
    g_current_state = ST_INIT;
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
        case ST_APPROACH:
            approach(signal, p1, p2);
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
        case ST_REVERSE_UTURN:
            reverse_uturn(signal, p1, p2);
            return;
        case ST_SEARCH_SCRAMBLE:
            search_scramble(signal, p1, p2);
            return;
    }
}

