#include "midi_hw_if.h"
#include "defs.h" 
#include "heap.h" 

typedef struct {
    uint32_t counts[MIDI_HW_IF_CHAN_MAX][MIDI_HW_IF_PITCH_MAX];
    midi_hw_if_flag_t flags;
} midi_hw_if_ev_filter_t;

struct midi_ev_seq_t {
    Heap *_h;
};

struct midi_hw_if_t {
    midi_hw_if_ev_filter_t evfilter;
    midi_ev_seq_t *midi_ev_seq;
};

static void 
midi_hw_if_ev_free (midi_hw_if_ev_t *se) 
{ 
    free((void*)se); 
}

/* make min heap where equal elements allowed */
static int midi_ev_seq_event_cmp (void *a, void *b)
{
    midi_hw_if_ev_t *a_ = (midi_hw_if_ev_t*)a;
    midi_hw_if_ev_t *b_ = (midi_hw_if_ev_t*)b;
    return a_->ts >= b_->ts;
}

/* Seq's heap doesn't need to know the index */
static void set_idx_ignore(void *a, size_t idx)
{
    return;
}

midi_ev_seq_t *
midi_ev_seq_new (size_t maxsize)
{
    midi_ev_seq_t *midi_ev_seq = NULL;
    midi_ev_seq = _C(midi_ev_seq_t,1);
    if (midi_ev_seq) {
        midi_ev_seq->_h = Heap_new(maxsize,
                midi_ev_seq_event_cmp,set_idx_ignore);
        if (!midi_ev_seq->_h) { _F(midi_ev_seq); return NULL; }
    }
    return midi_ev_seq;
}

void midi_ev_seq_free (midi_ev_seq_t *midi_ev_seq)
{
    Heap_free(midi_ev_seq->_h);
    _F(midi_ev_seq);
}

err_t midi_ev_seq_add_event(midi_ev_seq_t *s, midi_hw_if_ev_t *se)
{
    HeapErr err;
    err = Heap_push(s->_h,(void*)se);
    if (err == HEAP_EFULL) {
        return err_FULL;
    }
    if (err != HEAP_ENONE) {
        return err_EINVAL;
    }
    return err_NONE;
}

void
midi_ev_seq_play_up_to_time(midi_ev_seq_t *midi_ev_seq, 
        size_t time, void (*fun)(midi_hw_if_ev_t *, void*), void *aux)
{
    midi_hw_if_ev_t *se;
    for (se = (midi_hw_if_ev_t*)midi_ev_seq->_h->A[0];
         (se != NULL) && (se->time <= time);
         (void)Heap_pop(midi_ev_seq->_h,(void**)&se)) {
        fun(se,aux);
    }
}

static void
_midi_ev_seq_free_all_fun (midi_hw_if_ev_t *ev, void *aux)
{
    if (ev) { midi_hw_if_ev_free(ev); }
}

void midi_ev_seq_free_all_events(midi_ev_seq_t *midi_ev_seq)
{
    midi_ev_seq_play_up_to_time(midi_ev_seq,
            MIDI_HW_IF_TS_MAX,_midi_ev_seq_free_all_fun,NULL);
}

int
midi_hw_if_ev_filter_should_play (midi_hw_if_ev_filter_t *ef,
        midi_hw_if_ev_t *ev)
{
    switch (ev->type) {
        case midi_hw_if_ev_type_NOTEON:
            if ((ev->noteon.chan >= MIDI_HW_IF_CHAN_MAX)
                || (ev->noteon.pitch >= MIDI_HW_IF_PITCH_MAX)
                || ef->counts[ev->noteon.chan][ev->noteon.pitch] == UINT32_MAX) {
               return 0;
            }
            ef->counts[ev->noteon.chan][ev->noteon.pitch] += 1;
            if (ef->flags & midi_hw_if_flag_NOTEONS) {
                return ef->counts[ev->noteon.chan][ev->noteon.pitch] == 1 ? 1 : 0;
            }
            return 1;
        case midi_hw_if_ev_type_NOTEOFF:
            if ((ev->noteon.chan >= MIDI_HW_IF_CHAN_MAX)
                || (ev->noteon.pitch >= MIDI_HW_IF_PITCH_MAX)
                || ef->counts[ev->noteon.chan][ev->noteon.pitch] == 0) {
               return 0;
            }
            ef->counts[ev->noteon.chan][ev->noteon.pitch] -= 1;
            return ef->counts[ev->noteon.chan][ev->noteon.pitch] == 0 ? 1 : 0;
        default:
            return 1;
    }
}

typedef struct {
    midi_hw_if_ev_filter_t *evfilt;
    void (*fun)(midi_hw_if_ev_t *, void*);
    void *aux;
} _send_evs_t;

static void
_send_evs (midi_hw_if_ev_t *ev, void *aux)
{
    _send_evs_t *info = (_send_evs_t*)aux;
    if (midi_hw_if_ev_filter_should_play(info->mhi->evfilt,ev)) {
        info->fun(ev,info->aux);
    }
    /* event is freed. The caller had to make copies of the event if it wanted its
       info to stick around */
    _F(se);
}

/* Events are passed to fun, which should send them to the hardware.  The
   pointer to the event is no longer valid after the call to fun, so if info
   should stick around, the events must be copied by fun. */
void
midi_hw_if_send_evs (midi_hw_if_t *mhi, midi_hw_if_ts_t time,
        void (*fun)(midi_hw_if_ev_t *, void*), void *aux)
{
    _send_evs_t info = {
        .evfilt = &mhi->evfilt;
        .fun = fun;
        .aux = aux;
    };
    midi_ev_seq_play_up_to_time(mhi->midi_ev_seq, time, _send_evs, (void*)&info);
}

/* TODO: Add receive function */
    
midi_hw_if_t *
midi_hw_if_new (size_t maxevents, midi_hw_if_flag_t flags)
{
    midi_hw_if_t *ret = _C(1,sizeof(midi_hw_if_t));
    midi_ev_seq_t *msq = midi_ev_seq_new(maxevents);
    if (!msq) { _F(ret); return NULL; }
    ret->midi_ev_seq = msq;
}

void
midi_hw_if_free (midi_hw_if_t *mhi)
{
    midi_ev_seq_free(mhi->midi_ev_seq);
    _F(mhi);
}
