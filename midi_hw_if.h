#ifndef MIDI_HW_IF_H
#define MIDI_HW_IF_H

#include <stdint.h>

#define MIDI_HW_IF_PITCH_MAX 128
#define MIDI_HW_IF_CHAN_MAX 16

/* Time stamp. One time stamp unit is one nanosecond or 1.e-9 seconds. */
typedef uint64_t midi_hw_if_ts_t;
#define MIDI_HW_IF_TS_MAX UINT64_MAX

/* Event type */
typedef enum {
    midi_hw_if_ev_type_NOTEON,
    midi_hw_if_ev_type_NOTEOFF,
    /* TODO: Add more types. */
} midi_hw_if_ev_type_t;

typedef struct {
    midi_hw_if_ev_type_t type;
    union {
        struct {
            uint8_t chan;
            uint8_t pitch;
            uint8_t vel;
        } noteoff;
        struct {
            uint8_t chan;
            uint8_t pitch;
            uint8_t vel;
        } noteon;
        /* TODO: Add more types */
    };
    midi_hw_if_ts_t ts;
} midi_hw_if_ev_t;

static inline size_t
midi_hw_if_ev_data_len(midi_hw_if_ev_t *ev)
{
    switch (ev->type) {
        case midi_hw_if_ev_type_NOTEOFF:
            return 3;
        case midi_hw_if_ev_type_NOTEON:
            return 3;
        default:
            return 0;
    }
}

static inline void
midi_hw_if_ev_fill_data(midi_hw_if_ev_t *ev, char *data)
{
    switch (ev->type) {
        case midi_hw_if_ev_type_NOTEOFF:
            data[0] = 0x8 | (0xf & ev->noteoff.chan);
            data[1] = ev->noteoff.pitch;
            data[2] = ev->noteoff.vel;
            return;
        case midi_hw_if_ev_type_NOTEON:
            data[0] = 0x9 | (0xf & ev->noteon.chan);
            data[1] = ev->noteon.pitch;
            data[2] = ev->noteon.vel;
            return;
        default:
            return;
    }
}

typedef enum {
    /* Filter out repeated note ons, too. */
    midi_hw_if_flag_NOTEONS = 0x1,
} midi_hw_if_flag_t;

typedef struct midi_hw_if_t midi_hw_if_t;

midi_hw_if_t *
midi_hw_if_new(size_t            maxevents,
               midi_hw_if_flag_t flags,
               int (*mutex_lock)(void *mutex),
               int (*mutex_trylock)(void *mutex),
               int (*mutex_unlock)(void *mutex),
               void *mutex)
void
midi_hw_if_free(midi_hw_if_t *mhi);
void
midi_hw_if_send_evs(midi_hw_if_t *  mhi,
                    midi_hw_if_ts_t time,
                    void (*fun)(midi_hw_if_ev_t *, void *),
                    void *aux);

#endif /* MIDI_HW_IF_H */
