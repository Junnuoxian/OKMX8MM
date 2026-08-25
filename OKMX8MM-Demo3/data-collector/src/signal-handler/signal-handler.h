#ifndef __SIGNAL_HANDLER_H

#define __SIGNAL_HANDLER_H 1

int init_signal_handler();

void add_shutdown_callback(void (*shutdown_callback)());

void fire_sigint_handler();

#endif