/* Guaranteed to hold any formatted input (assuming at most 64-bit floats and
 * size_ts) */
#define NOTE_EVENT_BUF_LEN 100 
static const char *note_event_as_str (note_event_t *ne) {
    static char buf[NOTE_EVENT_BUF_LEN];
    seq_event_t *se = (seq_event_t*)ne;
    sprintf(buf,"note %zu %.4e %.4e %zu",se->time,ne->pitch,ne->vel,ne->len);
    return buf;
}

note_event_t *note_event_f_new_note(note_event_f_t *nef,
        size_t time,
        f32_t pitch,
        f32_t vel,
        size_t len)
{
    note_event_t *ne;
    seq_event_t *se;
    ne = _M(note_event_t,1);
    se = (seq_event_t*)ne
    if (!ne) { return ne; }
    seq_event_init_default(se,time);
    se->cb = nef->play_fun;
    se->data = nef->player;
    se->as_str = note_event_as_str;
    return ne;
}
