#ifndef MIDI_HW_IF_H
#define MIDI_HW_IF_H 

#include <stdint.h>

#define MIDI_HW_IF_PITCH_MAX 128
#define MIDI_HW_IF_CHAN_MAX 16 

/* Time stamp */
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
    } 
    midi_hw_if_ts_t ts;
} midi_hw_if_ev_t;

typedef enum {
    /* Filter out repeated note ons, too. */
    midi_hw_if_flag_NOTEONS = 0x1,
} midi_hw_if_flag_t;

typedef struct midi_hw_if_t midi_hw_if_t;

midi_hw_if_t *
midi_hw_if_new (size_t maxevents, midi_hw_if_flag_t flags);
void
midi_hw_if_free (midi_hw_if_t *mhi);
void
midi_hw_if_send_evs (midi_hw_if_t *mhi, midi_hw_if_ts_t time,
        void (*fun)(midi_hw_if_ev_t *, void*), void *aux);

#endif /* MIDI_HW_IF_H */
