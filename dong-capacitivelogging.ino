/**
* @author Stijn Boutsen
* @link boutsman.be
*
* Notes:
* Orginal code from: https://sites.google.com/site/bharatbhushankonka/home/diy-midi-over-usb-using-arduino-uno
* Has some quirks though
* 
*/

#include <CapacitiveSensor.h>
#define SENDPIN A1
#define TRESHOLD 220 //Sensitivity treshold for the capacitive surfaces
#define READPIN0 2  //Pin for pad1
#define READPIN1 3  //Pin for pad2
#define READPIN2 4  //Pin for pad3
#define READPIN3 5  //Pin for pad4
#define READPIN4 6  //Pin for pad5
#define READPIN5 7  //Pin for pad6
#define READPIN6 8  //Pin for pad7
#define READPIN7 9  //Pin for pad8
#define READPIN8 10  //Pin for pad9
#define READPIN9 11  //Pin for padext


#define MIDI_COMMAND_NOTE_ON 0x90
#define MIDI_COMMAND_NOTE_OF 0x80

const int aantalKnoppen = 10; //Aanpassen naargelang aantal knoppen

CapacitiveSensor   cs_2_0 = CapacitiveSensor(SENDPIN,READPIN0);        // 10M resistor between pins 4 & 2, pin 2 is sensor pin, add a wire and or foil if desired
CapacitiveSensor   cs_2_1 = CapacitiveSensor(SENDPIN,READPIN1);
CapacitiveSensor   cs_2_2 = CapacitiveSensor(SENDPIN,READPIN2);
CapacitiveSensor   cs_2_3 = CapacitiveSensor(SENDPIN,READPIN3);
CapacitiveSensor   cs_2_4 = CapacitiveSensor(SENDPIN,READPIN4);
CapacitiveSensor   cs_2_5 = CapacitiveSensor(SENDPIN,READPIN5);
CapacitiveSensor   cs_2_6 = CapacitiveSensor(SENDPIN,READPIN6);
CapacitiveSensor   cs_2_7 = CapacitiveSensor(SENDPIN,READPIN7);
CapacitiveSensor   cs_2_8 = CapacitiveSensor(SENDPIN,READPIN8);
CapacitiveSensor   cs_2_9 = CapacitiveSensor(SENDPIN,READPIN9);


//Array for storing the notes
//Each pad corresponds to a specific midi-note
//C4 M
//int note[aantalKnoppen] = {60, 62, 64, 65, 67, 69, 71, 72, 84, 86};  
//C3 M
int note[aantalKnoppen] = {48, 50, 52, 53, 55, 57, 59, 60, 72, 74};  
// C4m
//int note[aantalKnoppen] = {60, 62, 63, 65, 67, 68, 70, 72, 84, 86};  
// C3m
//int note[aantalKnoppen] = {48, 50, 51, 53, 55, 56, 58, 60, 72, 74};  

//Values of the Buttons
int val[aantalKnoppen];
int lastVal[aantalKnoppen];
int toets[aantalKnoppen];
int chan[aantalKnoppen] = {0,0};

// the format of the message to send Via serial 
typedef union {
    struct {
 uint8_t command;
 uint8_t note;
 uint8_t data;
    } msg;
    uint8_t raw[3];
} t_midiMsg;

t_midiMsg midiMsg2; //MIDI message for Buttons

void setup() {
  //Pin setup
  pinMode(READPIN0, INPUT);
  pinMode(READPIN1, INPUT);
  pinMode(READPIN2, INPUT);
  pinMode(READPIN3, INPUT);
  pinMode(READPIN4, INPUT);
  pinMode(READPIN5, INPUT);
  pinMode(READPIN6, INPUT);
  pinMode(READPIN7, INPUT);
  pinMode(READPIN8, INPUT);
  pinMode(READPIN9, INPUT);

  
  //Autocallibration
  //cs_2_0.set_CS_AutocaL_Millis(0xFFFFFFFF);
  //cs_2_1.set_CS_AutocaL_Millis(0xFFFFFFFF);
  //cs_2_2.set_CS_AutocaL_Millis(0xFFFFFFFF);

  //Start Serial Port
  Serial.begin(115200);
}

void loop(){

  const long beforeRead = millis();
  const uint8_t param = 70;
  
  //lectura 4 pads
  long temp[10];
  temp[0] = cs_2_0.capacitiveSensor(param);
  temp[1] = cs_2_1.capacitiveSensor(param);
  temp[2] = cs_2_2.capacitiveSensor(param);
  temp[3] = cs_2_3.capacitiveSensor(param);
  temp[4] = cs_2_4.capacitiveSensor(param);
  temp[5] = cs_2_5.capacitiveSensor(param);
  temp[6] = cs_2_6.capacitiveSensor(param);
  temp[7] = cs_2_7.capacitiveSensor(param);
  temp[8] = cs_2_8.capacitiveSensor(param);
  temp[9] = cs_2_9.capacitiveSensor(param);

  const long afterRead = millis();

  const long readingTime = afterRead-beforeRead;
  Serial.print("(");
  Serial.print(readingTime);
  Serial.print(",");
  Serial.print(temp[0]);
  Serial.println(")");
  
  for(int i=0;i<aantalKnoppen;i++)
  {
    convertToBoolean(temp[i], i);
    readDigital(i); //(inputPin, number of the button)
  }
}

void readDigital(int btn)
{
  //Digital Values
  val[btn] = toets[btn];

  if(val[btn] == 1){
     if(lastVal[btn] == 0){
       //Send status byte (status + messageType + channel)
        midiMsg2.msg.command = MIDI_COMMAND_NOTE_ON+chan[btn];
        //Send Data byte 1
        midiMsg2.msg.note    = note[btn];
        //Send Data byte 2
        midiMsg2.msg.data    = 127;

        /* Send note on */
        //Serial.write(midiMsg2.raw, sizeof(midiMsg2));        
     }
  }
  if(val[btn] == 0){
     if(lastVal[btn] == 1){
        midiMsg2.msg.command = MIDI_COMMAND_NOTE_OF+chan[btn];
        midiMsg2.msg.note    = note[btn];
        midiMsg2.msg.data    = 0;
            
        /* Send note off */
        //Serial.write(midiMsg2.raw, sizeof(midiMsg2));
     }
  }
  lastVal[btn] = val[btn];     
}

void convertToBoolean(int capsWaarde, int nr)
{
  if(capsWaarde > TRESHOLD)
  {
    toets[nr] = 1;
  }
  else
  {
    toets[nr] = 0;
  }
}
