/* A MIDI hardware interface for CoreMIDI */

#include "midi_hw_if.h"
#include <CoreMIDI/CoreMIDI.h>
#include <CoreServices/CoreServices.h>
#include <pthread.h>

#define STR_BUF_LEN 256
static char strbuf[STR_BUF_LEN];

/* The amount of time between polls of the host time.  This, in combination with
   the sched_period, dictate the responsiveness of the scheduler. */
#define MIDI_HW_IF_COREMIDI_USLEEP_TIME 1000 // check every millisecond

struct {
    midi_hw_if_t   *midi_hw_if;
    /* The thread on which MIDI events are sent to hardware */
    pthread_t       out_thread;
    /* The mutex for this thread so we can either be adding output events, or
       sending them */
    pthread_mutex_t mutex;
    /* The current scheduler time */
    midi_hw_if_ts_t cur_time;
    /* The amount of time to look ahead when scheduling */
    midi_hw_if_ts_t lookahead_time;
    /* The period of the scheduler */
    midi_hw_if_ts_t sched_period;
    volatile int    running;
    MIDIClientRef   client;
    MIDIPortRef     out_port;
    MIDIEndpointRef out_dest;
} midi_hw_if_coremidi_t;

/* Outputs the current time */
midi_hw_if_ts_t
midi_hw_if_coremidi_cur_time(void)
{
    /* conversion needs to be made.
       done without conversion functions because these marked as deprecated
       (see DriverServices.h). */
    static mach_timebase_info_data_t machtbinfo;
    if (machtbinfo.denom == 0) {
        (void)mach_timebase_info(&machtbinfo);
    }
    HostTime now = AudioGetCurrentHostTime();
    return (now / machtbinfo.denom) * machtbinfo.numer;
}

HostTime
midi_hw_if_ts_to_HostTime(midi_hw_if_ts_t ts)
{
    /* Depends what the timestamps mean */
    static mach_timebase_info_data_t machtbinfo;
    if (machtbinfo.denom == 0) {
        (void)mach_timebase_info(&machtbinfo);
    }
    return (ts / machtbinfo.numer) * machtbinfo.denom;
}

static void
_send_fun(midi_hw_if_ev_t *ev, void *aux)
{
    midi_hw_if_coremidi_t *mh = (midi_hw_if_coremidi_t *)aux;
    MIDIPacketList pktlist = {
        .numPackets = 1,
        .packet =
          (MIDIPacket){
            .timeStamp = midi_hw_if_ts_to_HostTime(ev->ts),
            .length = midi_hw_if_ev_data_len(ev),
          }
    };
    midi_hw_if_ev_fill_data(ev, pktlist.packet.data);
    OSSatus oss;
    if ((oss = MIDISend(mh->out_port, mh->out_dest, &pktlist))) {
        _PE("error %d sending MIDI data", oss);
    }
}

static void *
_out_thread(void *arg)
{
    midi_hw_if_coremidi_t *mh = (midi_hw_if_coremidi_t *)arg;
    while (mh->running) {
        while (midi_hw_if_coremidi_cur_time() < mh->cur_time) {
            usleep(MIDI_HW_IF_COREMIDI_USLEEP_TIME);
        }
        /* Try sending, if mutex locked, events will get sent next time. */
        (void)midi_hw_if_send_evs(mh->midi_hw_if,
                mh->cur_time + mh->lookahead_time,
                _send_fun,
                (void *)mh);
        mh->cur_time = midi_hw_if_coremidi_cur_time() + mh->sched_period;
    }
    return arg;
}

void
midi_hw_if_coremidi_free(midi_hw_if_coremidi_t *mh)
{
    if (mh) {
        (void)midi_hw_if_coremidi_stop(mh);
        pthread_mutex_destroy(&mh->mutex);
        if (mh->midi_hw_if) {
            midi_hw_if_free(mh->midi_hw_if);
        }
        if (mh->client) {
            MIDIClientDispose(mh->client);
        }
        if (mh->out_port) {
            MIDIPortDispose(mh->out_port);
        }
        if (mh->out_dest) {
            MIDIEndpointDispose(mh->out_dest);
        }
        _F(mh);
    }
}

static inline int
_mutex_lock(void *mutex)
{
    return pthread_mutex_lock((pthread_mutex_t*)mutex);
}

static inline int
_mutex_trylock(void *mutex)
{
    return pthread_mutex_trylock((pthread_mutex_t*)mutex);
}

static inline int
_mutex_unlock(void *mutex)
{
    return pthread_mutex_unlock((pthread_mutex_t*)mutex);
}

midi_hw_if_coremidi_t *
midi_hw_if_coremidi_new(size_t            maxevents,
                        midi_hw_if_flag_t flags,
                        char *            clientname,
                        char *            outportname,
                        midi_hw_if_ts_t   lookahead_time,
                        midi_hw_if_ts_t   sched_period)
{
    midi_hw_if_coremidi_t *ret = NULL;
    do {
        ret = _C(midi_hw_if_coremidi_t, 1);
        if (!ret) {
            break;
        }
        ret->mutex = PTHREAD_MUTEX_INITIALIZER;
        ret->midi_hw_if = midi_hw_if_new(maxevents, 
                flags, _mutex_lock, _mutex_trylock, _mutex_unlock, 
                (void*)&ret->mutex);
        if (!ret->midi_hw_if) {
            break;
        }
        OSStatus oss;
        if ((oss =
               MIDIClientCreate(CFSTR(clientname), NULL, NULL, &ret->client))) {
            _PE("error %d creating client %s", oss, clientname);
            break;
        }
        if ((oss = MIDIOutputPortCreate(client, outportname, &ret->out_port))) {
            _PE("error %d creating output port", oss);
            break;
        }
        ret->lookahead_time = lookahead_time;
        ret->sched_period = sched_period;
        return ret;
    } while (0);
    midi_hw_if_coremidi_free(ret);
    return NULL;
}

err_t
midi_hw_if_coremidi_set_out_dest(midi_hw_if_coremidi_t *mh, char *destname)
{
    ItemCount i;
    i = MIDIGetNumberOfDestinations();
    while (i--) {
        MIDIEndpointRef destref = MIDIGetDestination(i);
        CFStringRef     name;
        MIDIObjectGetStringProperty(destref, kMIDIPropertyName, &name);
        CFStringGetCString(name, strbuf, STR_BUF_LEN, kCFStringEncodingUTF8);
        if (strcmp(strbuf, destname) == 0) {
            mh->out_dest = destref;
            return err_NONE;
        }
    }
    return err_NFND;
}

err_t
midi_hw_if_coremidi_start(midi_hw_if_coremidi_t *mh)
{
    if (!mh->out_dest) {
        _PE("no out_dest set");
        return err_EINVAL;
    }
    mh->cur_time = midi_hw_if_coremidi_cur_time();
    mh->running = 1;
    pthread_create(&mh->out_thread, NULL, _out_thread, (void *)mh);
    return err_NONE;
}

err_t
midi_hw_if_coremidi_stop(midi_hw_if_coremidi_t *mh)
{
    if (!mh->running) {
        _PE("not running");
        return err_EINVAL;
    }
    mh->running = 0;
    /* wait for thread to finish */
    while (!pthread_join(mh->out_thread));
    return err_NONE;
}

/* This will probably go in a superclass eventually */
midi_hw_if_t *
midi_hw_if_coremidi_get_hw_if(midi_hw_if_coremidi_t *mh)
{
    return mh->midi_hw_if;
}
