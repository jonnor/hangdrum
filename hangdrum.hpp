
#include <stdint.h>
#include <stddef.h>

// for Parser. sscanf/memset
#include <stdio.h>
#include <string.h>

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

#ifdef HAVE_JSON11
const char * to_string(MidiMessageType t) {
    using T = MidiMessageType;

    switch (t) {
    case T::NoteOn: return "noteon";
    case T::NoteOff: return "noteoff";
    case T::Nothing: return "nothing";
    default: return "unknown";
    };  
}
#endif

struct MidiEventMessage {
    MidiMessageType type;
    int8_t channel;
    int8_t pitch;
    int8_t velocity;

#ifdef HAVE_JSON11
    json11::Json to_json() const {
        using namespace json11;
        return Json::object {
            {"type", to_string(type)},
            {"channel", channel},
            {"pitch", pitch},
            {"velocity", velocity},
        };
    }
#endif
};

static int
exponentialMovingAverage(const int value, const int previous, const float smoothening) {
    return (smoothening*value) + ((1-smoothening)*previous);
}  

// Simulation things
class Parser {

public:
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

// App things
struct PadConfig {
    const int8_t note;
    const int8_t velocity;
    const int8_t channel;
};

static const int N_PADS = 9;
struct Config {
    int8_t octave = 4;
    int8_t velocity = 64;
    int onthreshold = 30;
    int offthreshold = 20;
    float lowpass = 0.5;
    float highpass = 0.15;
    const PadConfig pads[N_PADS] = {
        { midiNote(Note::C, octave), velocity, 1 },
        { midiNote(Note::D, octave), velocity, 2 },
        { midiNote(Note::E, octave), velocity, 3 },
        { midiNote(Note::F, octave), velocity, 4 },
        { midiNote(Note::G, octave), velocity, 5 },
        { midiNote(Note::A, octave), velocity, 6 },
        { midiNote(Note::B, octave), velocity, 7 },
        { midiNote(Note::C, octave+1), velocity, 8 },
        { midiNote(Note::C, octave+2), velocity, 9 },
    };


#ifdef HAVE_JSON11
    Config(const json11::Json &json) {
        //Config config;
        json11::Json::object obj = json.object_items();
        for (auto& kv : obj) {
            const std::string key = kv.first;
            const auto &value = kv.second;
            if (key == "onthreshold") {
                this->onthreshold = value.number_value();
            } else if (key == "offthreshold") {
                this->offthreshold = value.number_value();
            } else if (key == "lowpass") {
                this->lowpass = value.number_value();
            } else if (key == "highpass") {
                this->highpass = value.number_value();
            } else {
                // unknown key
            }
        }
    }

    json11::Json to_json() const {
        using namespace json11;
        return Json::object {
            {"onthreshold", onthreshold},
            {"offthreshold", offthreshold},
            {"lowpass", lowpass},
            {"highpass", highpass},
        };
    }

#endif
};

enum class PadStateE : int8_t {
    StayOff = 11,
    TurnOn,
    StayOn,
    TurnOff,
};

struct PadState {
    int raw = 0;
    int lowpassed = 0;
    int highfilter = 0;
    int highpassed = 0;
    int value = 0;    
    PadStateE state = PadStateE::StayOff;
};

struct State {
    PadState pads[N_PADS];
    MidiEventMessage messages[N_PADS];
    long time;
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
    int capacitance;
};

struct Input {
    PadInput values[N_PADS];
    long time; // milliseconds
};

PadState
calculateStatePad(const PadState &previous, const PadInput input,
                  const PadConfig &config, const Config &appConfig) {
    using S = PadStateE;
    PadState next = previous;

    // Apply input
    next.raw = input.capacitance;
    next.highfilter = exponentialMovingAverage(input.capacitance, previous.highfilter, appConfig.highpass);
    next.highpassed = input.capacitance - next.highfilter;
    next.lowpassed = exponentialMovingAverage(next.highpassed, previous.lowpassed, appConfig.lowpass);
    next.value = next.raw;

    // Move from transient states to stables ones
    next.state = (next.state == S::TurnOn) ? S::StayOn : next.state;
    next.state = (next.state == S::TurnOff) ? S::StayOff : next.state;   

    // Change to new state
    if (next.state == S::StayOn) {
        if (next.value < appConfig.offthreshold) {
            next.state = S::TurnOff;
        }
    } else if (next.state == S::StayOff) {
        if (next.value > appConfig.onthreshold) {
            next.state = S::TurnOn;
        }
    } else {
        // XXX: Should never happen
    }
    return next;
}

void
calculateMidiMessages(const State &state, const Config &config,
    MidiEventMessage *buffer) {

    // TODO: check pre-condition buffer.size() == state.size() == config.size();

    for (unsigned int i=0; i<N_PADS; i++) {
        const auto & pad = state.pads[i];
        const auto & cfg = config.pads[i];
        buffer[i] = \
            { eventFromState(pad), cfg.channel, cfg.note, cfg.velocity };
    }
};

State
calculateState(const State &previous, const Input &input, const Config &config) {
    State next = previous;

    next.time = input.time;
    // TODO: check pre-condition, all array sizes are the same

    for (unsigned int i=0; i<N_PADS; i++) {
        next.pads[i] = calculateStatePad(previous.pads[i], input.values[i], config.pads[i], config);
    }

    calculateMidiMessages(next, config, next.messages);
    return next;
}

} // end namespace hangdrum
