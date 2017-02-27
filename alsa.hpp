#include <alsa/asoundlib.h>

namespace alsa {

struct Output {
private:
    int port;
    snd_seq_t *handle;
    int targetId;
    int targetPort;

public:
    void open(int id, int port)
    {
        targetId = id;
        targetPort = port;

        snd_seq_t *handle;
        const int err = snd_seq_open(&handle, "default", SND_SEQ_OPEN_OUTPUT, 0);
        if (err < 0) {
            throw std::invalid_argument("alsa.Output: Failed to open");
        }
        const int set_name = snd_seq_set_client_name(handle, "hangdrum-simulator");
        if (set_name < 0) {
            fprintf(stderr, "%s\n", snd_strerror(set_name));
        }
        this->handle = handle;
        this->port = snd_seq_create_simple_port(handle, "output",
                        SND_SEQ_PORT_CAP_SUBS_READ  | SND_SEQ_PORT_CAP_READ,
                        SND_SEQ_PORT_TYPE_APPLICATION);

        //const int connect = snd_seq_connect_to(handle, this->port, 131, 0);
        //printf("connect_to %d\n", connect);
        //snd_seq_alloc_named_queue(handle, "my queue");
    }

    void send(hangdrum::MidiEventMessage event)
    {
        snd_seq_event_t ev;
        snd_seq_ev_set_fixed(&ev);
        snd_seq_ev_clear(&ev);
        snd_seq_ev_set_direct(&ev); // don't use a queue
        snd_seq_ev_set_source(&ev, this->port);
        snd_seq_ev_set_dest(&ev, this->targetId, this->targetPort);
        //snd_seq_ev_set_subs(&ev);

        if (event.type == hangdrum::MidiMessageType::NoteOn) {
            snd_seq_ev_set_noteon(&ev,
                event.channel, event.pitch, event.velocity);
        } else if (event.type == hangdrum::MidiMessageType::NoteOff) {
            snd_seq_ev_set_noteoff(&ev,
                event.channel, event.pitch, event.velocity);
        } else {
            throw std::invalid_argument("alsa.Output: Unknown event type");
        }

        const int queued_events = snd_seq_event_output(this->handle, &ev);
        const int remaining_bytes = snd_seq_drain_output(this->handle);
        if (queued_events < 0 or remaining_bytes < 0) {
            throw std::runtime_error(std::string("Could not send event: ")
                                        + snd_strerror(queued_events));
        }
    }
};

} // end namespace alsa
