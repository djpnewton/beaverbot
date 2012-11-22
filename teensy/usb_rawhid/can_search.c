#include "can_search.h"

#include <stdlib.h>
#include <util/delay.h>

volatile enum search_states_t g_current_state = ST_INIT;
volatile int g_current_boom_back = 0;
enum search_mode_t g_search_mode = SM_PUSH;
cam_search_motor_set_t g_motor_set_callback;

#define BASE_DUTY_CYCLE 200
#define SPIN_DUTY_CYCLE 60
#define SPIN_SUPER_DUTY_CYCLE 250
#define SPIN_SUPER_TIMEOUT 120
#define REVERSE_TIMEOUT 600
#define SEARCH_TIMEOUT 3000
#define SCRAMBLE_TIMEOUT 500
#define DRIFTON_TIMEOUT 500
#define DRIFTONHARD_TIMEOUT 1500
#define DRIFTCENTER_TIMEOUT 100

volatile int g_reverse_time = -1;
volatile int g_search_time = -1;
volatile int g_scramble_time = -1;
volatile int g_drifton_time = -1;
volatile int g_driftonhard_time = -1;
volatile int g_driftcenter_time = -1;

#define DAMPER_SCALE 4.0f
void set_motors_from_can_pos(float x, float y, int base_duty_cycle)
{
    uint8_t value1 = base_duty_cycle;
    uint8_t value2 = base_duty_cycle;
    uint8_t diff;
    if (x < 0.5f)
    {
        value2 = (uint8_t)(x / 0.5f * base_duty_cycle);
        diff = base_duty_cycle - value2;
        value2 += (uint8_t)(diff * (DAMPER_SCALE - 1) / DAMPER_SCALE);
    }
    if (x > 0.5f)
    {
        value1 = (uint8_t)((x - 0.5f) / 0.5f * base_duty_cycle);
        diff = base_duty_cycle - value1;
        value1 += (uint8_t)(diff * (DAMPER_SCALE - 1) / DAMPER_SCALE);
    }
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

void set_motors_forwards_slow(void)
{
    g_motor_set_callback(1, BASE_DUTY_CYCLE * 2 / 3, 1, BASE_DUTY_CYCLE * 2 / 3);
}

void set_motors_forwards_left_hardish(void)
{
    g_motor_set_callback(1, BASE_DUTY_CYCLE, 1, BASE_DUTY_CYCLE * 5 / 7);
}

void set_motors_forwards_left_hard(void)
{
    g_motor_set_callback(1, BASE_DUTY_CYCLE, 1, BASE_DUTY_CYCLE * 5 / 9);
}

void set_motors_forwards_right(void)
{
    g_motor_set_callback(1, BASE_DUTY_CYCLE * 4 / 5, 1, BASE_DUTY_CYCLE);
}

void set_motors_backwards(void)
{
    g_motor_set_callback(2, BASE_DUTY_CYCLE, 2, BASE_DUTY_CYCLE - 80);
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

void search_wall(enum state_signals_t signal, float p1, float p2)
{
    switch (signal)
    {
        case SIG_ENTRY:
        {
            set_motors_forwards_right();
            g_search_time = 0;
            break;
        }
        case SIG_EXIT:
            g_search_time = -1;
            break;
        case SIG_CAN_SPOTTED:
            if (p1 > 0.35f && p1 < 0.65f && p2 > 0.75)
            {
                if (g_search_mode == SM_PUSH)
                    transition(ST_PUSH);
                else
                    transition(ST_SUPER_SPIN);
            }
            else
            {
                if (!g_current_boom_back)
                    set_motors_from_can_pos(p1, p2, BASE_DUTY_CYCLE);
                else
                    set_motors_from_can_pos(p1, p2, BASE_DUTY_CYCLE / 2);
                g_search_time = 0;
            }
            break;
        case SIG_FRONT_LEFT:
        case SIG_FRONT_RIGHT:
            if ((int)p1)
                transition(ST_REVERSE);
            break;
        case SIG_BOOM_FRONT:
            if ((int)p1)
                transition(ST_DRIFT_OFF_WALL);
            break;
        case SIG_BOOM_BACK:
            if ((int)p1)
                transition(ST_SPIN_OFF_WALL);
            break;
        case SIG_SEARCH_TIMEOUT:
            transition(ST_SEARCH_SCRAMBLE);
            break;
        default:
            break;
    }
}

void drift_off_wall(enum state_signals_t signal, float p1, float p2)
{
     switch (signal)
    {
        case SIG_ENTRY:
            set_motors_forwards_right();
            break;
        case SIG_CAN_SPOTTED:
            transition(ST_SEARCH_WALL);
            break;
        case SIG_FRONT_LEFT:
        case SIG_FRONT_RIGHT:
            if ((int)p1)
                transition(ST_REVERSE);
            break;
        case SIG_BOOM_FRONT:
            if (!(int)p1)
                transition(ST_DRIFT_TO_CENTER);
            break;
        case SIG_BOOM_BACK:
            if ((int)p1)
                transition(ST_SPIN_OFF_WALL);
            break;
        default:
            break;
    }
}

void drift_to_center(enum state_signals_t signal, float p1, float p2)
{
     switch (signal)
    {
        case SIG_ENTRY:
            g_driftcenter_time = 0;
            set_motors_forwards_right();
            break;
        case SIG_CAN_SPOTTED:
            transition(ST_SEARCH_WALL);
            break;
        case SIG_FRONT_LEFT:
        case SIG_FRONT_RIGHT:
            if ((int)p1)
                transition(ST_REVERSE);
            break;
        case SIG_BOOM_FRONT:
            if ((int)p1)
                transition(ST_DRIFT_OFF_WALL);
            break;
        case SIG_BOOM_BACK:
            if ((int)p1)
                transition(ST_SPIN_OFF_WALL);
            break;
        case SIG_DRIFT_CENTER_TIMEOUT:
            transition(ST_DRIFT_ON_WALL);
            break;
        default:
            break;
    }
}

void drift_on_wall(enum state_signals_t signal, float p1, float p2)
{
     switch (signal)
    {
        case SIG_ENTRY:
            g_drifton_time = 0;
            set_motors_forwards_left_hardish();
            break;
        case SIG_CAN_SPOTTED:
            transition(ST_SEARCH_WALL);
            break;
        case SIG_FRONT_LEFT:
        case SIG_FRONT_RIGHT:
            if ((int)p1)
                transition(ST_REVERSE);
            break;
        case SIG_BOOM_FRONT:
            if ((int)p1)
                transition(ST_DRIFT_OFF_WALL);
            break;
        case SIG_BOOM_BACK:
            if ((int)p1)
                transition(ST_SPIN_OFF_WALL);
            break;
        case SIG_DRIFT_ON_TIMEOUT:
            transition(ST_DRIFT_ON_WALL_HARD);
            break;
        default:
            break;
    }
}

void drift_on_wall_hard(enum state_signals_t signal, float p1, float p2)
{
     switch (signal)
    {
        case SIG_ENTRY:
            g_driftonhard_time = 0;
            set_motors_forwards_left_hard();
            break;
        case SIG_CAN_SPOTTED:
            transition(ST_SEARCH_WALL);
            break;
        case SIG_FRONT_LEFT:
        case SIG_FRONT_RIGHT:
            if ((int)p1)
                transition(ST_REVERSE);
            break;
        case SIG_BOOM_FRONT:
            if ((int)p1)
                transition(ST_DRIFT_OFF_WALL);
            break;
        case SIG_BOOM_BACK:
            if ((int)p1)
                transition(ST_SPIN_OFF_WALL);
            break;
        case SIG_DRIFT_ON_HARD_TIMEOUT:
            transition(ST_SEARCH_WALL);
            break;
        default:
            break;
    }
}

void spin_off_wall(enum state_signals_t signal, float p1, float p2)
{
     switch (signal)
    {
        case SIG_ENTRY:
            set_motors_spin_right();
            break;
        case SIG_FRONT_LEFT:
        case SIG_FRONT_RIGHT:
            if ((int)p1)
                transition(ST_REVERSE);
            break;
        case SIG_BOOM_FRONT:
            if (!(int)p1)
                transition(ST_DRIFT_TO_CENTER);
            break;
        case SIG_BOOM_BACK:
            if (!(int)p1)
                set_motors_forwards_right();
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
            transition(ST_SEARCH_WALL);
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
        case SIG_BOOM_FRONT:
        case SIG_BOOM_BACK:
            if ((int)p1)
                set_motors_forwards_slow();
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
            transition(ST_DRIFT_ON_WALL);
            break;
        case SIG_BACK_LEFT:
        case SIG_BACK_RIGHT:
            if ((int)p1)
                transition(ST_DRIFT_ON_WALL);
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
            transition(ST_SEARCH_WALL);
            break;
        case SIG_CAN_SPOTTED:
            transition(ST_SEARCH_WALL);
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
        transition(ST_SEARCH_WALL);
}

void can_search_signal(enum state_signals_t signal, float p1, float p2)
{
    if (signal == SIG_BOOM_BACK)
        g_current_boom_back = (int)p1;

    switch (g_current_state)
    {
        case ST_INIT:
            init(signal, p1, p2);
        case ST_SEARCH_WALL:
            search_wall(signal, p1, p2);
            return;
        case ST_DRIFT_OFF_WALL:
            drift_off_wall(signal, p1, p2);
            return;
        case ST_DRIFT_TO_CENTER:
            drift_to_center(signal, p1, p2);
            return;
        case ST_SPIN_OFF_WALL:
            spin_off_wall(signal, p1, p2);
            return;
        case ST_DRIFT_ON_WALL:
            drift_on_wall(signal, p1, p2);
            return;
        case ST_DRIFT_ON_WALL_HARD:
            drift_on_wall_hard(signal, p1, p2);
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
    if (g_drifton_time >= 0)
    {
        g_drifton_time++;
        if (g_drifton_time > DRIFTON_TIMEOUT)
        {
            can_search_signal(SIG_DRIFT_ON_TIMEOUT, 0, 0);
            g_drifton_time = -1;
        }
    }
    if (g_driftonhard_time >= 0)
    {
        g_driftonhard_time++;
        if (g_driftonhard_time > DRIFTONHARD_TIMEOUT)
        {
            can_search_signal(SIG_DRIFT_ON_HARD_TIMEOUT, 0, 0);
            g_driftonhard_time = -1;
        }
    }
    if (g_driftcenter_time >= 0)
    {
        g_driftcenter_time++;
        if (g_driftcenter_time > DRIFTCENTER_TIMEOUT)
        {
            can_search_signal(SIG_DRIFT_CENTER_TIMEOUT, 0, 0);
            g_driftcenter_time = -1;
        }
    }
}
