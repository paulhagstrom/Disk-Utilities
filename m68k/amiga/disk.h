/******************************************************************************
 * disk.h
 * 
 * Amiga disk handling.
 * 
 * Written in 2011 by Keir Fraser
 */

#ifndef __DISK_H__
#define __DISK_H__

struct amiga_state;
#include <amiga/event.h>

enum motor_state {
    motor_off, motor_spinning_up, motor_spinning_down, motor_on
};

enum step_state {
    step_none, step_in, step_out
};

struct disk {
    struct event *motor_delay;
    enum motor_state motor;

    struct event *step_delay;
    enum step_state step;

    uint8_t old_ciabb;

    uint16_t tracknr;

    int fd;
    unsigned char *buf;
    struct event *mfm_delay;
    time_ns_t last_mfm_bit_time;
    unsigned int mfmpos, bitpos, mfmbyte, ns_per_cell;
    uint16_t mfm_word;

    uint8_t dma;
    uint16_t dsklen;
};

void disk_init(struct amiga_state *);
void disk_cia_changed(struct amiga_state *);
void disk_dsklen_changed(struct amiga_state *);

#endif /* __DISK_H__ */
