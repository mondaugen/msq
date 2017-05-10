#ifndef NOTE_PLAYER_H
#define NOTE_PLAYER_H 
#include "note_event.h" 

typedef struct note_player_event_t note_player_event_t;
typedef struct note_player_t note_player_t;
void note_player_event_get_pitch_vel(note_player_event_t *npe,
                                     f32_t *pitch,
                                     f32_t *vel);

#endif /* NOTE_PLAYER_H */
