#ifndef NOTE_PLAYER_H
#define NOTE_PLAYER_H 
#include <stddef.h> 
#include "defs.h" 
#include "types.h" 
#include "note_event.h" 

FDS(note_player_event_t);
FDS(note_player_t);

void note_player_event_get_pitch_vel(note_player_event_t *npe,
                                     f32_t *pitch,
                                     f32_t *vel);
void note_player_process_note(note_player_t *np,
                              const note_event_t *ne);
void note_player_inc_time(note_player_t *np, size_t dtime);
void note_player_play_pending_note_offs(note_player_t *np);
note_player_t *
note_player_new(
        void (*note_on_from_event)(const note_event_t*,void*),
        void (*note_off_from_event)(const note_player_event_t*,
                                    void*),
        void *data,
        size_t max_pending_events);

#endif /* NOTE_PLAYER_H */
