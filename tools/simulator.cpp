
#include "../hangdrum.hpp"

#include <cstdio>
#include <vector>
#include <fstream>
#include <string>

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

class Parser {

struct DelayValue {
    int delay = -1;
    int value = -1;

    const bool valid() {
        return value > 0 and value < 2048 and delay > 0 and delay < 50;
    }
};

public:
  DelayValue push(uint8_t data) {
    if (index >= BUFFER_MAX) {
      // prevent overflowing buffer
      index = 0;
    }
    buffer[index++] = data;

    DelayValue ret;
    if (data == '\n') {
      sscanf(buffer, "(%d,%d)\n", &ret.delay, &ret.value);
      index = 0;
      memset(buffer, 0, BUFFER_MAX);
    }
    return ret;
  }
private:
  static const size_t BUFFER_MAX = 100;
  char buffer[BUFFER_MAX] = {};
  unsigned int index = 0;
};


std::vector<hangdrum::Input>
fake_events(std::vector<int> vals, int timeStepMs) {
    std::vector<hangdrum::Input> events;
    long currentTime = 0;
    for (int v : vals) {
        currentTime += timeStepMs;
        hangdrum::Input input {
            { v },
            currentTime,
        };
        events.push_back(input);
    }
    return events;
}

std::vector<hangdrum::Input> read_events(std::string filename) {
    std::vector<hangdrum::Input> events;
    long currentTime = 0;
    Parser parser;

    using charIterator = std::istreambuf_iterator<char>;
    std::ifstream filestream(filename);
    std::string content((charIterator(filestream)),(charIterator()));

    printf("read %s: %d\n", filename.c_str(), (int)content.size());
 
    for ( char &ch : content ) {
        auto val = parser.push(ch);
        if (not val.valid()) {
            continue;
        }
        currentTime += val.delay;
        hangdrum::Input input {
            { val.value },
            currentTime,
        };
        events.push_back(input);
    }

    printf("events: %d\n", (int)events.size());
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
    const bool realTime = true;

    const std::string filename = argv[1];
    std::vector<hangdrum::Input> inputEvents = read_events(filename);
    
    long currentTime = 0;
    for (auto &event : inputEvents) {
        if (realTime) {
            usleep(1000*(event.time-currentTime));
        }
        currentTime = event.time;

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
}
