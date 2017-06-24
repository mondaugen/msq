#include <stdio.h> 
#include "defs.h" 
#include "heap.h" 
#include "seq.h"

#define SEQ_END_STR_BUF_LEN 50  

struct seq_t {
    Heap *_h;
    seq_event_t *events;
};

/* make min heap where equal elements allowed */
static int seq_event_cmp (void *a, void *b)
{
    seq_event_t *a_ = (seq_event_t*)a;
    seq_event_t *b_ = (seq_event_t*)b;
    return a_->time >= b_->time;
}

/* Seq's heap doesn't need to know the index */
static void set_idx_ignore(void *a, size_t idx)
{
    return;
}

seq_t *seq_new (size_t maxsize)
{
    seq_t *seq = NULL;
    seq = _C(seq_t,1);
    if (seq) {
        seq->_h = Heap_new(maxsize,seq_event_cmp,set_idx_ignore);
        if (!seq->_h) { _F(seq); return NULL; }
    }
    return seq;
}

void seq_free (seq_t *seq)
{
    Heap_free(seq->_h);
    _F(seq);
}

err_t seq_add_event(seq_t *s, seq_event_t *se)
{
    HeapErr err;
    err = Heap_push(s->_h,(void*)se);
    if (err == HEAP_EFULL) {
        return err_FULL;
    }
    if (err != HEAP_ENONE) {
        return err_EINVAL;
    }
    /* Keep track of all events added */
    se->next = s->events;
    s->events = se;
    return err_NONE;
}

void seq_play_up_to_time(seq_t *seq, size_t time)
{
    seq_event_t *se;
    for (se = (seq_event_t*)seq->_h->A[0];
         (se != NULL) && (((seq_event_t*)se)->time <= time);
         (void)Heap_pop(seq->_h,(void**)&se)) {
    /* event not freed, might be added in again by the seq_end_t. time is
     * given to callback so that discrepancies between when event played and
     * when it was supposed to be played can be accounted for. */
        if (se->cb) { se->cb(seq,se,time); }
    }
}

void seq_free_all_events(seq_t *seq)
{
    /* clear all events from heap */
    Heap_clear(seq->_h);
    /* free events */
    seq_event_t *se;
    while (seq->events) {
        se = seq->events->next;
        seq->events->free(seq->events);
        seq->events = se;
    }
}

/* Call function on all events. Function cannot alter events. Data is passed
 * to function. */
void seq_map_events (seq_t *seq,
        void (*func)(seq_t *, const seq_event_t*, void*),
        void *data)
{
    seq_event_t *se = seq->events;
    while (se) { func(seq,se,data); se = se->next; }
}

static const char *seq_event_as_str (const seq_event_t *se)
{
    return "default";
}

static void seq_event_free (seq_event_t *se) { free((void*)se); }

void seq_event_init_default (seq_event_t *s, size_t time)
{
    _MZ(s,seq_event_t,1);
    s->free = seq_event_free;
    s->as_str = seq_event_as_str;
    s->time = time;
}

static void seq_end_cb (seq_t *s, seq_event_t *se, size_t time)
{
    seq_event_t *events = s->events;
    s->events = NULL;
    /* Push all events (they won't be in heap), will push se itself as well */
    while (events) {
        /* save next event, adding this event to seq will change it */
        seq_event_t *next = events->next;
        seq_add_event(s,events);
        events = next;
    }
    /* Data points to a size_t which gets updated with the reset time. This is
     * difference between time this was called and its scheduled time to account
     * for a late call. */
    *((size_t*)se->data) = time - se->time;
}

static const char *seq_end_as_str (const seq_event_t *se)
{
    static char buf[SEQ_END_STR_BUF_LEN];
    sprintf(buf,"loopend %zu",se->time);
    return buf;
}

/* event_time is when the event will occur (how long the loop is), clock
 * time is a pointer to a variable holding the time which will get reset when
 * the seq_end callback is called. */
seq_end_t *seq_end_new (size_t event_time, size_t *clock_time)
{
    /* currently no difference between seq_end_t and seq_event_t */
    seq_event_t *se = (seq_event_t*)_M(seq_end_t,1);
    if (!se) { return (seq_end_t*)se; }
    seq_event_init_default(se,event_time);
    se->cb = seq_end_cb;
    se->data = (void*)clock_time;
    se->as_str = seq_end_as_str;
    return (seq_end_t*)se;
}



