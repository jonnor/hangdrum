
#include <cstdint>
#include <vector>

namespace hangdrum {

enum class Note : int8_t {
    C=0, Csharp, D, Dsharp, E, 	F, 	Fs,	G, Gsharp, A, Asharp, B,
    Lenght = 12
};

static inline
int8_t midiNote(Note note, int8_t octave) {
    return (octave*12)+(int8_t)note;
}

struct PadConfig {
    const uint8_t pin;
    const int8_t note;
    const int8_t velocity = 64;
    const int threshold = 100; 
};

struct Config {
    const int8_t octave = 3;
    const int8_t channel = 0;
    const std::vector<PadConfig> pads {
        { 1, midiNote(Note::A, octave) },
        { 2, midiNote(Note::B, octave) },
        { 3, midiNote(Note::C, octave) },
    };
};

enum class PadStateE : int8_t {
    StayOff,
    TurnOn,
    StayOn,
    TurnOff,
};

struct PadState {
    int value = 0;
    PadStateE state = PadStateE::StayOff;
};

struct State {
    // XXX: should be able to declare size here and default-initialize?
    std::vector<PadState> pads = {
        PadState {},
        PadState {},
        PadState {},
    } ;
};

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

void
calculatePackets(const State &state, const Config &config, std::vector<MidiEventMessage> &buffer) {

    // FIXME: check pre-condition buffer.size() == state.size() == config.size();

    int number_messages = 0;
    for (int i=0; i<state.pads.size(); i++) {
        const auto & pad = state.pads[i];
        const auto & cfg = config.pads[i];
        const auto type = eventFromState(pad);
        buffer[i] = { type, config.channel, cfg.note, cfg.velocity };
    }
};

#ifdef ARDUINO
void
sendMessagesArduino(std::vector<MidiEventMessage> &messages) {
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
