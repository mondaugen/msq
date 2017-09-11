/* Test the midi hardware */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "midi_hw_if_coremidi.h"
#include "defs.h"

#define MIDIHW_MAX_EVENTS 1000
#define MIDIHW_SCHED_PERIOD 1000000UL /* Every ms */ 

static volatile int done = 0;

static void setdone (int sn)
{
    done = 1;
}

void usage (char *progname)
{
    fprintf(stderr,
            "Usage\n"
            "%s [options]\n"
            "   -d can be jack or coremidi\n"
            "   -o sets output destination\n"
            "   -h prints this message\n"
            "COREMIDI:\n"
            "   -c sets client name\n"
            "   -p sets port name\n",
            progname);
    exit(-1);
}

typedef struct {
    uint8_t pitch;
    uint8_t vel;
    midi_hw_if_ts_t start_time;
    midi_hw_if_ts_t len;
    uint8_t chan;
} note_desc_t;

static void
note_on_fill(midi_hw_if_ev_t *ev, void *aux)
{
    note_desc_t *nd = (note_desc_t*)aux;
    ev->type = midi_hw_if_ev_type_NOTEON;
    ev->noteon.chan = nd->chan;
    ev->noteon.pitch = nd->pitch;
    ev->noteon.vel = nd->vel;
    ev->ts = nd->start_time;
}

static void
note_off_fill(midi_hw_if_ev_t *ev, void *aux)
{
    note_desc_t *nd = (note_desc_t*)aux;
    ev->type = midi_hw_if_ev_type_NOTEOFF;
    ev->noteoff.chan = nd->chan;
    ev->noteoff.pitch = nd->pitch;
    ev->noteoff.vel = nd->vel;
    ev->ts = nd->start_time + nd->len;
}

int
main (int argc, char **argv)
{
    int ch = '?';
    char *outdestname = NULL;
    char *clientname = NULL;
    char *portname = NULL;
    midi_hw_if_t *midi_hw_if = NULL;
    enum {
        MIDI_HW_IF_NONE,
        MIDI_HW_IF_COREMIDI,
        MIDI_HW_IF_JACK
    } hwif = MIDI_HW_IF_NONE;
    while ((ch = getopt(argc,argv,"d:o:c:p:h")) != -1) {
        switch (ch) {
            case 'd':
                if ((strcmp(optarg,"jack") == 0)) {
                    hwif = MIDI_HW_IF_JACK;
                } else if ((strcmp(optarg,"coremidi") == 0)) {
                    hwif = MIDI_HW_IF_COREMIDI;
                }
                break;
            case 'o':
                outdestname = optarg;
                break;
            case 'c':
                clientname = optarg;
                break;
            case 'p':
                portname = optarg;
                break;
            case 'h':
            case '?':
                usage(argv[0]);
        }
    }
    switch (hwif) {
        case MIDI_HW_IF_NONE:
            printf("No hardware selected.\n");
            break;
        case MIDI_HW_IF_COREMIDI:
            printf("CoreMIDI hardware selected.\n");
            break;
        case MIDI_HW_IF_JACK:
            printf("JACK hardware selected.\n");
            break;
    }
    if (hwif == MIDI_HW_IF_COREMIDI) {
        midi_hw_if_coremidi_t *midi_hw_cm = midi_hw_if_coremidi_new(MIDIHW_MAX_EVENTS,
                0,
                clientname,
                portname,
                MIDIHW_SCHED_PERIOD,
                MIDIHW_SCHED_PERIOD);
        if (!midi_hw_cm) {
            _PE("making coremidi hardware instance "
                "with clientname %s and portname %s",
                clientname,
                portname);
        }
        if (!midi_hw_if_coremidi_set_out_dest(midi_hw_cm,outdestname)) {
            _PE("setting output destination to %s",outdestname);
        }
        midi_hw_if_coremidi_start(midi_hw_cm);
        midi_hw_if = midi_hw_if_coremidi_get_hw_if(midi_hw_cm); 
        note_desc_t nd = {
            .vel = 100,
            .start_time = midi_hw_if_get_cur_time(midi_hw_if),
            .len = 1e9*0.5,
            .chan = 0
        };
        uint8_t pitches[] = {60,64,67};
        size_t i;
        for (i = 0; i < sizeof(pitches)/sizeof(uint8_t); i++) {
            nd.pitch = pitches[i];
            midi_hw_if_sched_ev(midi_hw_if,note_on_fill,(void*)&nd);
            nd.start_time += nd.len;
            midi_hw_if_sched_ev(midi_hw_if,note_off_fill,(void*)&nd);
        }
    }
    signal(SIGINT,setdone);
    while (!done);
    exit(0);
}
