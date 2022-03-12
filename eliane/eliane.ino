/**
   eliane is based on eighties_dystopia.ino, 27 Dec 2021 - @todbot
   12 March 2022 - @ef1j95

    Details: 
    - Originally a swirling ominous wub that evolves over time
    - Running on Teensy 3.2 using ADC pin 14
    - As with orignial, no user input

    Circuit for this implementation:
    - A14/DAC to tip of TRS audio jack (L)
    - GND to base of TRS audio jack
      TRS 1/8" (3.5mm) headphone plug

    Code:
    - Five detuned oscillators are randomly detuned every second or so
    - A low-pass filter is slowly modulated over the filters
    - The filter modulation rate also changes randomly about every second
    - Every 52.5 seconds, a new note is randomly chosen from the allowed note list
    - The new note is slowly mixed in with the old note as it is faded out
    - 100% hack

*/
#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/saw_analogue512_int8.h> // oscillator waveform
#include <tables/cos2048_int8.h> // filter modulation waveform
#include <LowPassFilter.h>
#include <mozzi_rand.h>  // for rand()
#include <mozzi_midi.h>  // for mtof()

Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOsc1(SAW_ANALOGUE512_DATA);
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOsc2(SAW_ANALOGUE512_DATA);
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOsc3(SAW_ANALOGUE512_DATA);
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOsc4(SAW_ANALOGUE512_DATA);
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOsc5(SAW_ANALOGUE512_DATA);
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOsc6(SAW_ANALOGUE512_DATA);
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOsc7(SAW_ANALOGUE512_DATA);
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOsc8(SAW_ANALOGUE512_DATA);
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOsc9(SAW_ANALOGUE512_DATA);
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> aOsc10(SAW_ANALOGUE512_DATA);
Oscil<COS2048_NUM_CELLS, CONTROL_RATE> kFilterMod(COS2048_DATA);

LowPassFilter lpf;
uint8_t resonance = 170; // range 0-255, 255 is most resonant

//uint8_t notes[] = {33, 43}; // possible notes to play MIDI A1, A1#, G1
//uint8_t notes[] = {33, 34, 31}; // possible notes to play MIDI A1, A1#, G1
uint8_t notes[] = {33, 34, 31, 32, 35, 36, 37, 38, 39, 40, 41, 42, 43}; // the whole octave from G1 to G2
uint32_t note_duration = 17500*3;
uint8_t note2_id = notes[0];
uint8_t note1_id = notes[0];
uint32_t lastMillis = 0;
uint32_t slidelen = 17;
uint32_t slide = AUDIO_RATE*slidelen;
uint8_t toggle = 0;

void setup() {
  Serial.begin(115200);
  kFilterMod.setFreq(0.08f);
  lpf.setCutoffFreqAndResonance(20, resonance);
  setNotes();
  Serial.println(sizeof(notes)/sizeof(notes[0]));
  startMozzi();
}

// This is the default arduino loop. It should just call audioHook().
void loop() {
  audioHook();
}

void setNotes() {
  float fo = mtof(notes[note1_id]);
  float fn = mtof(notes[note2_id]);
  aOsc2.setFreq( fo + (float)rand(100) / 100); // orig 1.001, 1.002, 1.004
  aOsc3.setFreq( fo + (float)rand(100) / 100);
  aOsc4.setFreq( fo + (float)rand(100) / 100);
  aOsc5.setFreq( (fo / 2) + (float)rand(100) / 1000);
  aOsc6.setFreq( fn + (float)rand(100) / 100); // orig 1.001, 1.002, 1.004
  aOsc7.setFreq( fn + (float)rand(100) / 100);
  aOsc8.setFreq( fn + (float)rand(100) / 100);
  aOsc9.setFreq( (fn / 2) + (float)rand(100) / 1000);
}

// mozzi function, called every CONTROL_RATE
void updateControl() {
  // filter range (0-255) corresponds with 0-8191Hz
  // oscillator & mods run from -128 to 127
  byte cutoff_freq = 67 + kFilterMod.next() / 2;
  lpf.setCutoffFreqAndResonance(cutoff_freq, resonance);

  if (rand(CONTROL_RATE) == 0) { // about once every second
    Serial.println("!");
    kFilterMod.setFreq((float)rand(255) / 4096); // choose a new modulation frequency
    setNotes(); // wiggle the tuning a little
  }

  if ( millis() - lastMillis > note_duration )  {
    lastMillis = millis();
    toggle++;
    if (toggle % 2) {
      note2_id = rand(sizeof(notes)/sizeof(notes[0])); // number of notes in notes[]    
    }
    else {
      note1_id = rand(sizeof(notes)/sizeof(notes[0])); // number of notes in notes[]    
    }
    // (note_id+1) % 3;
    Serial.println((byte)notes[note1_id]);
    Serial.println((byte)notes[note2_id]);
    slide = AUDIO_RATE*slidelen;
    setNotes(); 
  }
}

// mozzi function, called every AUDIO_RATE to output sample
int updateAudio() {
  long asig;
  float mix = (float)slide/((float)(AUDIO_RATE*slidelen));
  if ( toggle %2 ) {
     asig = lpf.next((mix)*(aOsc1.next() +
                       aOsc2.next() +
                       aOsc3.next() +
                       aOsc4.next() +
                       aOsc5.next()) +
                       (1.-mix)*(aOsc10.next() +
                       aOsc6.next() +
                       aOsc7.next() +
                       aOsc8.next() +
                       aOsc9.next())
                      );
  }
  else {
     asig = lpf.next((1.-mix)*(aOsc1.next() +
                       aOsc2.next() +
                       aOsc3.next() +
                       aOsc4.next() +
                       aOsc5.next()) +
                       (mix)*(aOsc10.next() +
                       aOsc6.next() +
                       aOsc7.next() +
                       aOsc8.next() +
                       aOsc9.next())
                      );
  }
   if (slide > 0) slide--;
  return MonoOutput::fromAlmostNBit(11, asig);
}
