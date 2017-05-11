#ifndef SEQ_H
#define SEQ_H 

#include <stdlib.h> 
#include "err.h" 

struct seq_t;
typedef struct seq_t seq_t;

typedef struct seq_event_t {
    struct seq_event_t *next;
    size_t time;
    void (*cb)(seq_t*,struct seq_event_t*,size_t);
    void *data;
    void (*free)(struct seq_event_t*);
    const char *(*as_str)(const struct seq_event_t*);
} seq_event_t;

typedef struct seq_end_t {
    seq_event_t super;
    size_t len;
} seq_end_t;

seq_t *seq_new (size_t maxsize);
void seq_free (seq_t *seq);
err_t seq_add_event(seq_t *s, seq_event_t *se);
void seq_play_up_to_time(seq_t *seq, size_t time);
void seq_free_all_events(seq_t *seq);
void seq_map_events (seq_t *seq,
        void (*func)(seq_t *, const seq_event_t*, void*),
        void *data);
void seq_event_init_default (seq_event_t *s, size_t time);
seq_end_t *seq_end_new (size_t event_time, size_t *clock_time);
#endif /* SEQ_H */
