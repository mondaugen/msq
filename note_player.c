struct note_player_event_t {
    size_t _curidx;
    size_t _time;
    f32_t pitch;
    f32_t vel;
};

struct note_player_t {
    Heap *_h; /* pending note offs */
    size_t curtime;
    void (*note_on_from_event)(note_player_event_t*,void*);
    void (*note_off_from_event)(note_player_event_t*,void*);
    void *data;
};

void note_player_event_get_pitch_vel(note_player_event_t *npe,
        f32_t *pitch, f32_t *vel)
{
    *pitch = npe->pitch; *vel = npe->vel;
}

/* Heap stuff */

static int event_cmp (void *a, void *b)
{
    note_player_event_t *a_, *b_;
    a_ = (note_player_event_t*)a;
    b_ = (note_player_event_t*)b;
    return a_->_time >= b_->_time;
}

static void event_set_idx(void *a, size_t idx)
{
    note_player_event_t *a_;
    a_ = (note_player_event_t*)a;
    a_->_curidx = idx;
}

/* if returns non-zero, caller responsible for freeing npe */
static int event_insert(Heap *h, note_player_event_t *npe)
{
    /* check if event of same pitch in heap */
    void *ptr = h->A;
    size_t n;
    for (n = 0; n < h->size; n++) {
        if ((((note_player_event_t*)*ptr)->pitch == npe->pitch)
               && (((note_player_event_t*)*ptr)->_time < npe->_time)) {
           /* if timestamp of old event less than the timestamp of the new event, get rid
            * of the old event (free it) and put this one in its place */
            npe->_curidx = ((note_player_event_t*)*ptr)->_curidx;
            _F(*ptr);
            *ptr = npe;
            Heap_heapify(h,npe->_curidx);
            return 0; /* success */
        }
        ptr++;
    }
    /* pitch not there, push new event */
    if (Heap_push(h,(void*)npe) != HEAP_ENONE) {
        /* error pushing, this sucks but you will have a stuck
         * note. avoid by having a large enough heap and a way to kill all
         * notes in an emergency.*/
        return -1;
    }
    return 0; /* success */
}

/* note event can be freed after calling this, all info needed is copied */
void note_player_process_note(note_player_t *np, const note_event_t *ne)
{
    np->note_on_from_event(ne,np->data);
    note_player_event_t *npe = _M(note_player_event_t,1);
    *npe = (note_player_event_t) {
        ._time = np->curtime + ne->len,
        .pitch = ne->pitch,
        .vel = np->vel
    };
    if (event_insert(np->_h,npe)) {
        _F(npe); /* error inserting, free */
    }
}

void note_player_inc_time(note_player_t *np, size_t dtime)
{
    np->curtime += dtime;
}

void note_player_play_pending_note_offs(note_player_t *np)
{
    while (((note_player_event_t*)np->_h->A[0])->_time <=
            np->curtime) {
        note_player_event_t *npe = Heap_pop(np->_h);
        np->note_off_from_event(npe,np->data);
        _F(npe);
    }
}

/* data passed to note_on_from_event and note_off_from_event when called */
note_player_t *note_player_new(
        void (*note_on_from_event)(note_player_event_t*,void*),
        void (*note_off_from_event)(note_player_event_t*,void*),
        void *data,
        size_t max_pending_events)
{
    note_player_t *np = _M(note_player_t,1);
    if (!np) { return np; }
    np->_h = Heap_new(max_pending_events,
                      event_cmp,
                      event_set_idx);
    if (!np->_h) { _F(np); return NULL; }
    np->curtime = 0;
    np->note_on_from_event = note_on_from_event;
    np->note_off_from_event = note_off_from_event;
    np->data = data;
    return np;
}
