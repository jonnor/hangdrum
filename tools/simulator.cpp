
#include <json11.hpp>
#include <json11.cpp>

#include "../hangdrum.hpp"
#include "../flowtrace.hpp"
#include "../alsa.hpp"

#include <cstdio>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

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
create_flowtrace(const std::vector<hangdrum::State> &history,
                const hangdrum::Config &config) {

    std::vector<flowtrace::Event> events;

    time_t basetime;
    time(&basetime);

    const std::string graphData = read_file("./graphs/pad.json");
    std::string parseError;
    const json11::Json graph = json11::Json::parse(graphData, parseError);

    // IIPs
    flowtrace::Event th(config.onthreshold);
    th.time = isotime(basetime, 0);
    th.tgt = { "trigger", "onthreshold" };
    events.push_back(th);

    th.data = config.offthreshold;
    th.tgt = { "trigger", "offthreshold" };
    events.push_back(th);

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

void
simulatorRealizeState(const hangdrum::State &state,
                      alsa::Output &out, bool realTime) {
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

int main(int argc, char *argv[]) {
    hangdrum::State state;
    alsa::Output out;

    if (argc < 2) {
        fprintf(stderr, "Usage: hangdrum-simulate ./inputs.log [{ \"onthreshold\": ... }] [trace.json]\n");
        return -1;
    }

    std::string trace_filename = "trace.json";

    json11::Json options;
    if (argc > 2) {
        std::string err;
        options = json11::Json::parse(argv[2], err);
    }
    hangdrum::Config config(options);

    if (argc > 3) {
        trace_filename = argv[3];
    }

    std::cout << json11::Json(config).dump() << std::endl;

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

        simulatorRealizeState(state, out, realTime);
    }
    std::cout << "creating flowtrace" << std::endl;
    auto trace = create_flowtrace(states, config);
    write_flowtrace(trace_filename, trace);
    std::cout << "wrote trace to: " << trace_filename << std::endl;
}
