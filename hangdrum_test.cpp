
#include "./hangdrum.hpp"

#include <cstdio>
#include <vector>

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
            throw std::invalid_argument
("alsa.Output: Unknown event type");
        }

        const int queued_events = snd_seq_event_output(this->handle, &ev);
        const int remaining_bytes = snd_seq_drain_output(this->handle);

        if (queued_events < 0) {
            fprintf(stderr, "%s\n", snd_strerror(queued_events));
        }

        printf("alsa.Output sent event. %d, %d\n", queued_events, remaining_bytes);
    }
};

} // end namespace alsa




// read in the recorded input data
// play through it
// record what happened

std::vector<hangdrum::Input> read_events(std::string filename) {
    std::vector<hangdrum::Input> events;

    auto vals = { 0, 3, 2, 101, 103, 90, 120, 90, 120, 0, 0 };
    for (int v : vals) {
        hangdrum::Input input {
            { v },
        };
        events.push_back(input);
    }
    return events;
}

int main(int argc, char *argv[]) {
    const hangdrum::Config config;
    hangdrum::State state;
    alsa::Output out;

    if (argc < 2) {
        fprintf(stderr, "Usage: hangdrum-simulate ./inputs.log\n");
        return -1;
    }

    out.open(14, 0);

    const std::string filename = argv[1];
    std::vector<hangdrum::Input> inputEvents = read_events(filename);
    
    for (auto &event : inputEvents) {
        //printf("in: %d\n", event.values[0]);
        state = hangdrum::calculateState(state, event, config);
        //printf("state: %d\n", state.pads[0].state);
        //printf("value: %d\n", state.pads[0].value);

        for ( auto & m : state.messages ) {
            if (m.type != hangdrum::MidiMessageType::Nothing) {
                out.send(m);
            }

            switch (m.type) {
            case hangdrum::MidiMessageType::Nothing:
                // ignored, no-op
                break;
            case hangdrum::MidiMessageType::NoteOn:
                printf("NOTE ON: %d\n", m.pitch);
                break;
            case hangdrum::MidiMessageType::NoteOff:
                printf("NOTE OFF: %d\n", m.pitch);
                break;
            default:
                printf("WARNING: Unknown MIDI message %d\n", (int)m.type);
                break;
            }
        }
    }

    sleep(5);
}
