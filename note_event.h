#ifndef NOTE_EVENT_H
#define NOTE_EVENT_H 

#include <stddef.h> 
#include "types.h" 
#include "seq.h" 

typedef struct note_event_t {
    seq_event_t super;
    f32_t pitch;
    f32_t vel;
    size_t len;
} note_event_t;

/* A note event "factory". Produces notes that have pitch, vel, len that are
 * executed by the provided player and player's function */
typedef struct note_event_f_t {
    void (*play_fun)(seq_t*,seq_event_t*,size_t);
    void *player;
} note_event_f_t;
/* For initialization simply provide the function and the player. */

note_event_t *note_event_f_new_note(note_event_f_t *nef,
        size_t time,
        f32_t pitch,
        f32_t vel,
        size_t len);

#endif /* NOTE_EVENT_H */
