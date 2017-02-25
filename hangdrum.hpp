
#include <cstdint>
#include <vector>
#include <array>

namespace hangdrum {

enum class Note : int8_t {
    C=0, Csharp, D, Dsharp, E, 	F, 	Fs,	G, Gsharp, A, Asharp, B,
    Lenght = 12
};

static inline
int8_t midiNote(Note note, int8_t octave) {
    return (octave*12)+(int8_t)note;
}

enum class MidiMessageType : int8_t {
    Nothing = -1, // XXX: using optional/maybe type would be nicer 
    NoteOn = 0x09,
    NoteOff = 0x08,
};

struct MidiEventMessage {
    MidiMessageType type;
    int8_t channel;
    int8_t pitch;
    int8_t velocity;
};


// App things
struct PadConfig {
    const uint8_t pin;
    const int8_t note;
    const int8_t velocity = 64;
    const int threshold = 100; 
};

static const int N_PADS = 3;
struct Config {
    const int8_t octave = 3;
    const int8_t channel = 0;
    const std::vector<PadConfig> pads {
        { 1, midiNote(Note::A, octave) },
        { 2, midiNote(Note::B, octave) },
        { 3, midiNote(Note::C, octave) }
    };
};

enum class PadStateE : int8_t {
    StayOff = 11,
    TurnOn,
    StayOn,
    TurnOff,
};

struct PadState {
    int value = 0;
    PadStateE state = PadStateE::StayOff;
};

struct State {
    // XXX: number of pads much match config
    std::array<PadState, N_PADS> pads;
    std::array<MidiEventMessage, N_PADS> messages;
};

MidiMessageType
eventFromState(const PadState &pad) {
    using S = PadStateE;
    using E = MidiMessageType;

    switch (pad.state) {
    case S::TurnOn:
        return E::NoteOn;
    case S::TurnOff:
        return E::NoteOff;
    case S::StayOn:
        return E::Nothing;
    case S::StayOff:
        return E::Nothing;
    // no default, should be exhaustive
    }
};

struct Input {
    // XXX: size much match Config.pads.size()
    std::array<int, N_PADS> values;
};

PadState
calculateStatePad(const PadState &previous, const int input, const PadConfig &config) {
    using S = PadStateE;
    PadState next = previous;

    // Apply input
    next.value = input; // no filtering

    // Move from transient states to stables ones
    next.state = (previous.state == S::TurnOn) ? S::StayOn : previous.state;
    next.state = (previous.state == S::TurnOff) ? S::StayOff : previous.state;   

    // Change to new state
    if (next.state == S::StayOn) {
        if (next.value < config.threshold) {
            next.state = S::TurnOff;
        }
    } else if (next.state == S::StayOff) {
        if (next.value > config.threshold) {
            next.state = S::TurnOn;
        }
    } else {
        // XXX: Should never happen
    }
}

void
calculateMidiMessages(const State &state, const Config &config,
    std::array<MidiEventMessage, N_PADS> &buffer) {

    // FIXME: check pre-condition buffer.size() == state.size() == config.size();

    for (int i=0; i<state.pads.size(); i++) {
        const auto & pad = state.pads[i];
        const auto & cfg = config.pads[i];
        buffer[i] = \
            { eventFromState(pad), config.channel, cfg.note, cfg.velocity };
    }
};

State
calculateState(const State &previous, const Input &input, const Config &config) {
    State next = previous;

    // FIXME: check pre-condition, all array sizes are the same

    for (int i=0; i<next.pads.size(); i++) {
        next.pads[i] = calculateStatePad(previous.pads[i], input.values[i], config.pads[i]);
    }

    calculateMidiMessages(next, config, next.messages);
}

#ifdef ARDUINO
void
sendMessagesArduino(std::array<MidiEventMessage, N_PADS> &messages) {
    for ( const auto & m : messages ) {
        if (m.type == MidiEventMessage::Nothing) {
            continue;
        }
        const midiEventPacket_t packet = {
            MidiMessageType::NoteOn,
            0x80 | m.channel, m.pitch, m.velocity
        };
        MidiUSB.sendMIDI(packet);
    }
    MidiUSB.flush();
}
#endif

} // end namespace hangdrum
