
#include "./hangdrum.hpp"

#include <cstdio>
#include <vector>

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

    if (argc < 2) {
        fprintf(stderr, "Usage: hangdrum-simulate ./inputs.log\n");
        return -1;
    }

    const std::string filename = argv[1];
    std::vector<hangdrum::Input> inputEvents = read_events(filename);
    
    for (auto &event : inputEvents) {
        //printf("in: %d\n", event.values[0]);
        state = hangdrum::calculateState(state, event, config);
        //printf("state: %d\n", state.pads[0].state);
        //printf("value: %d\n", state.pads[0].value);

        for ( auto & m : state.messages ) {
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
