
#include <CapacitiveSensor.h>

#include "./hangdrum.hpp"
using namespace hangdrum;

Config config;
struct Sensors {
  std::array<CapacitiveSensor, N_PADS> capacitive;
} sensors;
State state;

#if 0
void
sendMessagesMidiUSB(std::array<MidiEventMessage, N_PADS> &messages) {
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

void setup() {
  //Pin setup
  for (int i=0; i<config.pads.size(); i++) {
    sensors.capacitive[i] = CapacitiveSensor(config.readPin, pad.pin);
    pinMode(pad.pin, INPUT);
  }

  Serial.begin(115200);
}

Input readInputs() {
  Input current;

  const uint8_t param = 20;
  current.time = millis();
  for (int i=0; i<config.pads.size(); i++) {
    const int val = sensors.capacitive[i].capacitiveSensor(param);
    current.values.capacitive[i] = val;
  }
}

void loop(){
  const long beforeRead = millis();
  Input input = readInputs();
  const long afterRead = millis();

  state = hangdrum::calculateState(state, event, config);
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

