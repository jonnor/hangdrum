
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

static const int N_PADS = 10;
struct Config {
    const int8_t octave = 4;
    const int8_t channel = 0;
    const int8_t sendPin = 1; // analog
    const std::array<PadConfig, N_PADS> pads {{
        { 2, midiNote(Note::C, octave) },
        { 3, midiNote(Note::D, octave) },
        { 4, midiNote(Note::E, octave) },
        { 5, midiNote(Note::F, octave) },
        { 6, midiNote(Note::G, octave) },
        { 7, midiNote(Note::A, octave) },
        { 8, midiNote(Note::B, octave) },
        { 9, midiNote(Note::C, octave+1) },
        { 10, midiNote(Note::C, octave+2) },
        { 11, midiNote(Note::D, octave+2) }, // pad ext
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
    long time; // milliseconds
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

} // end namespace hangdrum
