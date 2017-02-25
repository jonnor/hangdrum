
#include <cstdint>
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
    const std::array<PadConfig, N_PADS> pads {{
        { 1, midiNote(Note::A, octave) },
        { 2, midiNote(Note::B, octave) },
        { 3, midiNote(Note::C, octave) },
    }};
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
    default:
        return E::Nothing;
    }
};

struct PadInput {
    int capacitance = 0;
};

struct Input {
    std::array<PadInput, N_PADS> values;
};

PadState
calculateStatePad(const PadState &previous, const PadInput input, const PadConfig &config) {
    using S = PadStateE;
    PadState next = previous;

    // Apply input
    next.value = input.capacitance; // no filtering

    // Move from transient states to stables ones
    next.state = (next.state == S::TurnOn) ? S::StayOn : next.state;
    next.state = (next.state == S::TurnOff) ? S::StayOff : next.state;   

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
    return next;
}

void
calculateMidiMessages(const State &state, const Config &config,
    std::array<MidiEventMessage, N_PADS> &buffer) {

    // TODO: check pre-condition buffer.size() == state.size() == config.size();

    for (unsigned int i=0; i<state.pads.size(); i++) {
        const auto & pad = state.pads[i];
        const auto & cfg = config.pads[i];
        buffer[i] = \
            { eventFromState(pad), config.channel, cfg.note, cfg.velocity };
    }
};

State
calculateState(const State &previous, const Input &input, const Config &config) {
    State next = previous;

    // TODO: check pre-condition, all array sizes are the same

    for (unsigned int i=0; i<next.pads.size(); i++) {
        next.pads[i] = calculateStatePad(previous.pads[i], input.values[i], config.pads[i]);
    }

    calculateMidiMessages(next, config, next.messages);
    return next;
}

#ifdef ARDUINO
void
sendMessagesArduino(std::array<MidiEventMessage, N_PADS> &messages) {
    for ( const auto & m : messages ) {
        if (m.type == MidiEventMessage::Nothing) {
            continue;
        }
        const midiEventPacket_t packet = {
            m.type, 0x80 | m.channel, m.pitch, m.velocity
        };
        MidiUSB.sendMIDI(packet);
    }
    MidiUSB.flush();
}
#endif

} // end namespace hangdrum
