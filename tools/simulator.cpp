
#include <json11.hpp>
#include <json11.cpp>

#include "../hangdrum.hpp"
#include "../flowtrace.hpp"

#include <cstdio>
#include <vector>
#include <fstream>
#include <string>
#include <iostream>
#include <iomanip>

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

std::string read_file(std::string filename) {
    using charIterator = std::istreambuf_iterator<char>;
    std::ifstream filestream(filename);
    std::string content((charIterator(filestream)),(charIterator()));
    return content;
}

std::vector<hangdrum::Input> read_events(std::string filename) {
    std::vector<hangdrum::Input> events;
    long currentTime = 0;
    hangdrum::Parser parser;

    std::string content = read_file(filename);

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

std::string isotime(time_t basetime, long millis) {
    // XXX: HACK
    const long seconds = millis/1000;
    const long milliseconds = millis%1000;
    const time_t time = basetime + seconds;
    char buf[sizeof "2011-10-08T07:07:09.999Z"];
    const size_t len = strftime(buf, sizeof buf, "%FT%TZ", gmtime(&time));
    std::string str = std::string(buf);

    std::ostringstream ms;
    ms << "." << std::setw(3) << std::setfill('0') << milliseconds << "Z";
    str.replace(len-1, len+4, ms.str());
    return str;
}

json11::Json
create_flowtrace(const std::vector<hangdrum::State> &history) {

    std::vector<flowtrace::Event> events;

    time_t basetime;
    time(&basetime);

    const std::string graphData = read_file("./graphs/pad.json");
    std::string parseError;
    const json11::Json graph = json11::Json::parse(graphData, parseError);

    for (auto &state : history) {
        using Ev = flowtrace::Event;
        const auto &p = state.pads[0]; // we just care about single/first set
        // TODO: take time into account here

        const std::string time = isotime(basetime, state.time);

        Ev in(p.raw);
        in.time = time;
        in.src = { "read", "out" };
        in.tgt = { "lowpass", "in" };

        Ev lowpassed(p.lowpassed);
        lowpassed.time = time;
        lowpassed.src = { "lowpass", "out" };
        lowpassed.tgt = { "highpass", "in" };

        Ev highpassed(p.highpassed);
        highpassed.time = time;
        highpassed.src = { "highpass", "out" };
        highpassed.tgt = { "trigger", "in" };

        //Ev padstate(p.state);
        // padstate.time = time;

        events.push_back(in);
        events.push_back(lowpassed);
        events.push_back(highpassed);

        for (int i=0; i<hangdrum::N_PADS; i++) {
            auto &m = state.messages[i];
            Ev note(m);
            note.time = time;
            note.src = { "note", "out" };
            note.tgt = { "send", "in" };
            if (m.type != hangdrum::MidiMessageType::Nothing) {
                events.push_back(note);
            }
        }

    }

    // TODO: put a graph representation into the header
    using namespace json11;
    return Json::object {
        { "header", Json::object {
            { "graphs", Json::object {
                { "default", graph },
            }},
        }},
        { "events", events },
    };

}

void
write_flowtrace(std::string filename, const json11::Json &trace) {
    const std::string serialized = trace.dump();
    std::ofstream out(filename);
    out << serialized;
    out.close();
}

int main(int argc, char *argv[]) {
    const hangdrum::Config config;
    hangdrum::State state;
    alsa::Output out;

    if (argc < 2) {
        fprintf(stderr, "Usage: hangdrum-simulate ./inputs.log\n");
        return -1;
    }

    const bool realTime = false;
    if (realTime) {
        out.open(14, 0);
    }

    const std::string filename = argv[1];
    std::vector<hangdrum::Input> inputEvents = read_events(filename);
    std::vector<hangdrum::State> states;    

    long currentTime = 0;
    for (auto &event : inputEvents) {
        if (realTime) {
            usleep(1000*(event.time-currentTime));
        }
        currentTime = event.time;

        state = hangdrum::calculateState(state, event, config);
        states.push_back(state);

        for ( auto & m : state.messages ) {
            if (m.type != hangdrum::MidiMessageType::Nothing) {
                if (realTime) {
                    out.send(m);
                }
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
    std::cout << "creating flowtrace" << std::endl;
    auto trace = create_flowtrace(states);
    const std::string trace_filename = "trace.json"; 
    write_flowtrace(trace_filename, trace);
    std::cout << "wrote trace to: " << trace_filename << std::endl;
}
