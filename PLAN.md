# MIDI Sequencer plan

## Sequencer

Manages a collection of note events, eventually control change events, organized
in time. A single sequence contains events belonging to the same channel.

Provides methods 
    - adding/removing notes
    - changing the loop / end points
    - dumping event information
    - eventually storing other kinds of events, like control changes

## Note Recorder

Keep track of note ons and note offs and the time between them. When a note off
received after a corresponding note on, store this note as having happened with
when it happened and how long.

Implementation
After MIDI messages have been parsed with this recorder, make the events
available by passing in a callback which should use the event information, e.g.,
add the note to a sequence at the correct time.
Needs a clock to operate.

## Sequence player

Very simple object that uses a clock to play a sequence and manages this
process.

## Note player

Sends MIDI notes and keeps track of when the note offs should happen. Needs
clock.

## Controller

To this symbols and corresponding function and argument lists are attached.
For convenience the argument lists are specified as options, the -FAST option
used to specify this list in a given order or if an initial character of _ (or
something) the rest of the message unpacked as if all the types stored
contiguously.
