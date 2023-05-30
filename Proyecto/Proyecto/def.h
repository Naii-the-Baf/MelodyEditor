#ifndef DEF_H
#define DEF_H

#define btn_delay_ms 5UL
#define F_CPU 1000000UL
#define BAUDRATE 9600UL
#define BAUD_PRESCALE ((F_CPU/16UL/BAUDRATE) - 1)
#define BAUD_FAST_PRESCALE ((F_CPU/8UL/BAUDRATE) - 1)

#endif