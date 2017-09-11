#ifndef MIDI_HW_IF_COREMIDI_H
#define MIDI_HW_IF_COREMIDI_H

#include <CoreFoundation/CoreFoundation.h>
#include "midi_hw_if.h"

typedef struct midi_hw_if_coremidi_t midi_hw_if_coremidi_t;

UInt64
midi_hw_if_ts_to_HostTime(midi_hw_if_ts_t ts);
void
midi_hw_if_coremidi_free(midi_hw_if_coremidi_t *mh);
midi_hw_if_coremidi_t *
midi_hw_if_coremidi_new(size_t            maxevents,
                        midi_hw_if_flag_t flags,
                        char *            clientname,
                        char *            outportname,
                        midi_hw_if_ts_t   lookahead_time,
                        midi_hw_if_ts_t   sched_period);
err_t
midi_hw_if_coremidi_set_out_dest(midi_hw_if_coremidi_t *mh, char *destname);
err_t
midi_hw_if_coremidi_start(midi_hw_if_coremidi_t *mh);
err_t
midi_hw_if_coremidi_stop(midi_hw_if_coremidi_t *mh);
midi_hw_if_t *
midi_hw_if_coremidi_get_hw_if(midi_hw_if_coremidi_t *mh);
#endif /* MIDI_HW_IF_COREMIDI_H */
