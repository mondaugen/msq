#include "midi_hw_if.h"
#include "defs.h"
#include "heap.h"

typedef struct {
    uint32_t          counts[MIDI_HW_IF_CHAN_MAX][MIDI_HW_IF_PITCH_MAX];
    midi_hw_if_flag_t flags;
} midi_hw_if_ev_filter_t;

typedef struct {
    Heap *_h;
} midi_ev_seq_t;

struct midi_hw_if_t {
    midi_hw_if_ev_filter_t evfilter;
    midi_ev_seq_t *        midi_ev_seq;
    int (*mutex_lock)(void *mutex);
    int (*mutex_trylock)(void *mutex);
    int (*mutex_unlock)(void *mutex);
    void *mutex;
    midi_hw_if_ts_t (*get_cur_time)(struct midi_hw_if_t *);
};

static midi_hw_if_ev_t *
midi_hw_if_ev_new(void)
{
    return _M(midi_hw_if_ev_t, 1);
}

static void
midi_hw_if_ev_free(midi_hw_if_ev_t *se)
{
    _F((void *)se);
}

/* make min heap such that equal items are popped out in FIFO fashion*/
static int
midi_ev_seq_event_cmp(void *a, void *b)
{
    midi_hw_if_ev_t *a_ = (midi_hw_if_ev_t *)a;
    midi_hw_if_ev_t *b_ = (midi_hw_if_ev_t *)b;
    return a_->ts > b_->ts;
}

/* Seq's heap doesn't need to know the index */
static void
set_idx_ignore(void *a, size_t idx)
{
    return;
}

midi_ev_seq_t *
midi_ev_seq_new(size_t maxsize)
{
    midi_ev_seq_t *midi_ev_seq = NULL;
    midi_ev_seq = _C(midi_ev_seq_t, 1);
    if (midi_ev_seq) {
        midi_ev_seq->_h =
          Heap_new(maxsize, midi_ev_seq_event_cmp, set_idx_ignore);
        if (!midi_ev_seq->_h) {
            _F(midi_ev_seq);
            return NULL;
        }
    }
    return midi_ev_seq;
}

void
midi_ev_seq_free(midi_ev_seq_t *midi_ev_seq)
{
    Heap_free(midi_ev_seq->_h);
    _F(midi_ev_seq);
}

err_t
midi_ev_seq_add_event(midi_ev_seq_t *s, midi_hw_if_ev_t *se)
{
    HeapErr err;
    err = Heap_push(s->_h, (void *)se);
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
                            size_t         time,
                            void (*fun)(midi_hw_if_ev_t *, void *),
                            void *aux)
{
    midi_hw_if_ev_t *se = NULL;
    HeapErr          err;
    while ((err = Heap_top(midi_ev_seq->_h, (void **)&se)) == HEAP_ENONE) {
        /* This is actually redundant to check... */
        if (!se) {
            return;
        }
        if (se->ts <= time) {
            (void)Heap_pop(midi_ev_seq->_h, (void **)&se);
            fun(se, aux);
        } else {
            return;
        }
    }
}

static void
_midi_ev_seq_free_all_fun(midi_hw_if_ev_t *ev, void *aux)
{
    if (ev) {
        midi_hw_if_ev_free(ev);
    }
}

void
midi_ev_seq_free_all_events(midi_ev_seq_t *midi_ev_seq)
{
    midi_ev_seq_play_up_to_time(
      midi_ev_seq, MIDI_HW_IF_TS_MAX, _midi_ev_seq_free_all_fun, NULL);
}

void
midi_hw_if_ev_filter_init(midi_hw_if_ev_filter_t *ef, midi_hw_if_flag_t flags)
{
    // TODO: When possibilities for flags grow, need to mask flags not pertinent
    // to the ev_filter
    memset(ef, 0, sizeof(midi_hw_if_ev_filter_t));
    ef->flags |= flags;
}

int
midi_hw_if_ev_filter_should_play(midi_hw_if_ev_filter_t *ef,
                                 midi_hw_if_ev_t *       ev)
{
    switch (ev->type) {
        case midi_hw_if_ev_type_NOTEON:
            if ((ev->noteon.chan >= MIDI_HW_IF_CHAN_MAX) ||
                (ev->noteon.pitch >= MIDI_HW_IF_PITCH_MAX) ||
                ef->counts[ev->noteon.chan][ev->noteon.pitch] == UINT32_MAX) {
                return 0;
            }
            ef->counts[ev->noteon.chan][ev->noteon.pitch] += 1;
            if (ef->flags & midi_hw_if_flag_NOTEONS) {
                return ef->counts[ev->noteon.chan][ev->noteon.pitch] == 1 ? 1
                                                                          : 0;
            }
            return 1;
        case midi_hw_if_ev_type_NOTEOFF:
            if ((ev->noteon.chan >= MIDI_HW_IF_CHAN_MAX) ||
                (ev->noteon.pitch >= MIDI_HW_IF_PITCH_MAX) ||
                ef->counts[ev->noteon.chan][ev->noteon.pitch] == 0) {
                return 0;
            }
            ef->counts[ev->noteon.chan][ev->noteon.pitch] -= 1;
            if (ef->flags & midi_hw_if_flag_NOTEOFFS) {
                return ef->counts[ev->noteon.chan][ev->noteon.pitch] == 0 ? 1
                                                                          : 0;
            }
        default: return 1;
    }
}

typedef struct {
    midi_hw_if_ev_filter_t *evfilt;
    void (*fun)(midi_hw_if_ev_t *, void *);
    void *aux;
} _send_evs_t;

static void
_send_evs(midi_hw_if_ev_t *ev, void *aux)
{
    _send_evs_t *info = (_send_evs_t *)aux;
    if (midi_hw_if_ev_filter_should_play(info->evfilt, ev)) {
        info->fun(ev, info->aux);
    }
    /* event is freed. The caller had to make copies of the event if it wanted
       its info to stick around */
    _F(ev);
}

/* Events are passed to fun, which should send them to the hardware.  The
   pointer to the event is no longer valid after the call to fun, so if info
   should stick around, the events must be copied by fun.  Events having time
   stamps up to time are passed to fun.  Returns err_BUSY if mutex locked, in
   that case, the caller should wait and call again with the time waited added
   to time. */
err_t
midi_hw_if_send_evs(midi_hw_if_t *  mhi,
                    midi_hw_if_ts_t time,
                    void (*fun)(midi_hw_if_ev_t *, void *),
                    void *aux)
{
    _send_evs_t info = {.evfilt = &mhi->evfilter, .fun = fun, .aux = aux };
    if (mhi->mutex_trylock(mhi->mutex)) {
        return err_BUSY;
    }
    midi_ev_seq_play_up_to_time(
      mhi->midi_ev_seq, time, _send_evs, (void *)&info);
    if (mhi->mutex_unlock(mhi->mutex)) {
        return err_EINVAL;
    }
    return err_NONE;
}

/* fun will be passed a pointer to an event, which it can fill in. This event
   will get scheduled.  Returns non-zero on error, zero otherwise. aux is data
   that will be passed to the function in addition to the event. */
err_t
midi_hw_if_sched_ev(midi_hw_if_t *mhi,
                    void (*fun)(midi_hw_if_ev_t *, void *),
                    void *aux)
{
    /* Wait until mutex available */
    if (mhi->mutex_lock(mhi->mutex)) {
        return err_EINVAL;
    }
    midi_hw_if_ev_t *ev = midi_hw_if_ev_new();
    fun(ev, aux);
    err_t err = midi_ev_seq_add_event(mhi->midi_ev_seq, ev);
    if (err) {
        midi_hw_if_ev_free(ev);
        return err;
    }
    if (mhi->mutex_unlock(mhi->mutex)) {
        return err_EINVAL;
    }
    return err_NONE;
}

midi_hw_if_ts_t
midi_hw_if_get_cur_time(midi_hw_if_t *mh)
{
    return mh->get_cur_time(mh);
}

midi_hw_if_t *
midi_hw_if_new(midi_hw_if_new_t *mhn)
{
    midi_hw_if_t * ret = _C(midi_hw_if_t, 1);
    midi_ev_seq_t *msq = midi_ev_seq_new(mhn->maxevents);
    if (!msq) {
        _F(ret);
        return NULL;
    }
    ret->midi_ev_seq = msq;
    midi_hw_if_ev_filter_init(&ret->evfilter, mhn->flags);
    ret->mutex_lock = mhn->mutex_lock;
    ret->mutex_trylock = mhn->mutex_trylock;
    ret->mutex_unlock = mhn->mutex_unlock;
    ret->mutex = mhn->mutex;
    ret->get_cur_time = mhn->get_cur_time;
    return ret;
}

void
midi_hw_if_free(midi_hw_if_t *mhi)
{
    midi_ev_seq_free(mhi->midi_ev_seq);
    _F(mhi);
}

err_t
midi_hw_if_note_ev_set_start(midi_hw_if_note_ev_t *nev, midi_hw_if_ev_t *noev)
{
    if (noev->type != midi_hw_if_ev_type_NOTEON) {
        return err_EINVAL;
    }
    nev->noteon.pitch = noev->noteon.pitch;
    nev->noteon.vel = noev->noteon.vel;
    mev->ts = noev->ts;
    return err_NONE;
}

err_t
midi_hw_if_note_ev_set_end(midi_hw_if_note_ev_t *nev, midi_hw_if_ev_t *noev)
{
    if (noev->type != midi_hw_if_ev_type_NOTEOFF) {
        return err_EINVAL;
    }
    nev->noteoff.pitch = noev->noteoff.pitch;
    nev->noteoff.vel = noev->noteoff.vel;
    mev->len = noev->ts - nev->ts;
    return err_NONE;
}
