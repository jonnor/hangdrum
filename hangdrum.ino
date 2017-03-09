
#include <CapacitiveSensor.h>
#include <MIDIUSB.h>

#include "./hangdrum.hpp"
using namespace hangdrum;

Config config;
struct Sensors {
  CapacitiveSensor capacitive[N_PADS] = {
    CapacitiveSensor(config.sendPin, config.pads[0].pin),
    CapacitiveSensor(config.sendPin, config.pads[1].pin),
    CapacitiveSensor(config.sendPin, config.pads[2].pin),
    CapacitiveSensor(config.sendPin, config.pads[3].pin),
    CapacitiveSensor(config.sendPin, config.pads[4].pin),
    CapacitiveSensor(config.sendPin, config.pads[5].pin),
    CapacitiveSensor(config.sendPin, config.pads[6].pin),
    CapacitiveSensor(config.sendPin, config.pads[7].pin),
    CapacitiveSensor(config.sendPin, config.pads[8].pin),
    CapacitiveSensor(config.sendPin, config.pads[9].pin),
  };
};
Sensors sensors;
State state;
Parser parser;

#ifndef EMULATE_INPUTS
void sendMessagesMidiUSB(MidiEventMessage *messages) {
    int packetsSent = 0;
    for (int i=0; i<N_PADS; i++) {
        const auto &m = messages[i];
        midiEventPacket_t packet;
        if (m.type == MidiMessageType::Nothing) {
            continue;
        } else if (m.type == MidiMessageType::NoteOn) {
            packet = { 0x09, 0x90 | m.channel, m.pitch, m.velocity };
        } else if (m.type == MidiMessageType::NoteOff) {
            packet = { 0x08, 0x80 | m.channel, m.pitch, m.velocity };
        };
        MidiUSB.sendMIDI(packet);
        packetsSent++;
    }
    if (packetsSent) {
      MidiUSB.flush();
    }
}
#endif

Input readInputs() {
  Input current;

  const uint8_t param = 10;
  current.time = millis();
  for (int i=0; i<N_PADS; i++) {
    const int val = sensors.capacitive[i].capacitiveSensor(param);
    if (val >= 0) {
      current.values[i].capacitance = val;
    } else {
      // error/timeout
      current.values[i].capacitance = 0;
    }
  }
  return current;
}

void setup() {
  //Pin setup
  for (int i=0; i<N_PADS; i++) {
    auto &pad = config.pads[i];
    pinMode(pad.pin, INPUT);
    auto &s = sensors.capacitive[i];
    s.set_CS_Timeout_Millis(2);
  }
  pinMode(config.sendPin, OUTPUT);

  Serial.begin(115200);
}

long previousSend = 0;
void loop(){
  const long beforeRead = millis();
  const Input input = readInputs();
  const long afterRead = millis();

  state = hangdrum::calculateState(state, input, config);
  const long afterCalculation = millis();

  while (Serial.available()) {
    Parser::DelayValue val = parser.push(Serial.read());
    if (val.valid()) {
      const long beforeCalculation = millis();
      Input input = {
          { val.value },
          beforeCalculation
      };
      State next = state;
      for (int i=0; i<100; i++) { // waste, to be able to profile
         next = hangdrum::calculateState(state, input, config);
      }
      state = next;
      const long afterCalculation = millis();

      Serial.print("calculating: ");
      Serial.println(afterCalculation-beforeCalculation);
      Serial.flush();
    } else {
      Serial.println("w");
    }

  }

#ifndef EMULATE_INPUTS
  sendMessagesMidiUSB(state.messages);
#endif
  const long afterSend = millis();
  const long readingTime = afterRead-previousSend;
  previousSend = millis();

  Serial.print("(");
  Serial.print(readingTime);
  Serial.print(",");
  Serial.print(input.values[0].capacitance);
  Serial.println(")");

  // Note: if nothing reads the MIDI port, sending takes 250ms!
  //Serial.print("sending time: ");
  //Serial.println(afterSend-afterCalculation);
}

