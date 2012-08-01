#ifndef _CAN_SEARCH_H_
#define _CAN_SEARCH_H_

#include <stdint.h>

enum search_states_t
{
    ST_INIT,
    ST_SEARCH,
    ST_PUSH,
    ST_PUSH_LEFT,
    ST_PUSH_RIGHT,
    ST_REVERSE,
    ST_SPIN,
    ST_SEARCH_SCRAMBLE
};

enum state_signals_t
{
    SIG_ENTRY = 0,
    SIG_EXIT,
    SIG_USER,
    SIG_INIT,
    SIG_START_SEARCH,
    SIG_CAN_SPOTTED,
    SIG_FRONT_LEFT,
    SIG_FRONT_RIGHT,
    SIG_BACK_LEFT,
    SIG_BACK_RIGHT,
    SIG_SEARCH_TIMEOUT,
    SIG_SCRAMBLE_TIMEOUT,
    SIG_REVERSE_TIMEOUT,
    SIG_SPIN_TIMEOUT
};

typedef void (*cam_search_motor_set_t)(uint8_t direction1, uint8_t value1, uint8_t direction2, uint8_t value2); 

void can_search_init(cam_search_motor_set_t motor_set_callback);
void can_search_signal(enum state_signals_t signal, float p1, float p2);
void can_search_tick(void);

#endif
