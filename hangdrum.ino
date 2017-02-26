
#include <CapacitiveSensor.h>
#include <MIDIUSB.h>

#include "./hangdrum.hpp"
using namespace hangdrum;

Config config;
struct Sensors {
  CapacitiveSensor capacitive[1] = {
    CapacitiveSensor(config.sendPin, config.pads[0].pin),
  };
};
Sensors sensors;
State state;

void sendMessagesMidiUSB(MidiEventMessage *messages) {
    for (int i=0; i<N_PADS; i++) {
        const auto &m = messages[i];
        if (m.type == MidiMessageType::Nothing) {
            continue;
        }
        const midiEventPacket_t packet = {
            (uint8_t)m.type,
            0x80 | m.channel,
            m.pitch,
            m.velocity,
        };
        MidiUSB.sendMIDI(packet);
    }
    MidiUSB.flush();
}

Input readInputs() {
  Input current;

  const uint8_t param = 20;
  current.time = millis();
  for (int i=0; i<N_PADS; i++) {
    const int val = sensors.capacitive[i].capacitiveSensor(param);
    current.values[i].capacitance = val;
  }
}

void setup() {
  //Pin setup
  for (int i=0; i<N_PADS; i++) {
    auto &pad = config.pads[i];
    pinMode(pad.pin, INPUT);
  }

  Serial.begin(115200);
}

void loop(){
  const long beforeRead = millis();
  Input input = readInputs();
  const long afterRead = millis();

  state = hangdrum::calculateState(state, input, config);
  const long afterCalculation = millis();
  sendMessagesMidiUSB(state.messages);
  const long afterSend = millis();
/*
  const long readingTime = afterRead-beforeRead;
  Serial.print("(");
  Serial.print(readingTime);
  Serial.print(",");
  Serial.print(temp[0]);
  Serial.println(")");
*/

}

