#!/bin/bash
clang midi_hw_if.c midi_hw_if_coremidi.c heap.c \
    test/midi_hw_if_test.c -o test/bin/midi_hw_if_test \
    -framework CoreFoundation -framework CoreMIDI \
    -framework CoreAudio \
    -fconstant-cfstrings \
    -I.
