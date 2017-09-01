/* Test the midi hardware */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
//#include "midi_hw_if_coremidi.h"

void usage (char *progname)
{
    fprintf(stderr,
            "Usage\n"
            "%s [options]\n"
            "   -d can be jack or coremidi\n"
            "   -h prints this message\n",
            progname);
    exit(-1);
}

int
main (int argc, char **argv)
{
    int ch;
    enum {
        MIDI_HW_IF_NONE,
        MIDI_HW_IF_COREMIDI,
        MIDI_HW_IF_JACK
    } hwif = MIDI_HW_IF_NONE;
    while ((ch = getopt(argc,argv,"d:h")) != -1) {
        switch (ch) {
            case 'd':
                if ((strcmp(optarg,"jack") == 0)) {
                    hwif = MIDI_HW_IF_JACK;
                } else if ((strcmp(optarg,"coremidi") == 0)) {
                    hwif = MIDI_HW_IF_COREMIDI;
                }
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
    exit(0);
}
