//////// variables and libraries//////
#include "DaisyDuino.h"

//on harp
//A2 is middle

//setup pir
const int sensor = D14;
int state = LOW;
float peeps = 1;

//envelope
static AdEnv adenv;


//envelope for noise
static AdEnv adenvnoise;

//daisy
DaisyHardware hw;
size_t num_channels;
//reverb
ReverbSc verb;

//setup filter
static Svf filter;

// OscillatorBank osc[3];
static Oscillator oscillator01;

//noise
static WhiteNoise nse;
float noiseVolume = 0.5f;
float noiseMap;
float filterCutoff;

// uint8_t knob_pin = A1;

float verbValue;
int mappedValue;
//////////////smoothing////////////////////////////
// Define the number of samples to keep track of. The higher the number, the
// more the readings will be smoothed, but the slower the output will respond to
// the input. Using a constant rather than a normal variable lets us use this
// value to determine the size of the readings array.
const int numReadings = 10;

int readings[numReadings];  // the readings from the analog input
int readIndex = 0;          // the index of the current reading
int total = 0;              // the running total
int average = 0;            // the average

int inputPin = A2;

//plinkyness adenv set curve value from 100 to -100
int plinkyness;

////// synth setup ////////////

void setup() {
  Serial.begin(9600);
  pinMode(sensor, INPUT);
  //daisy setup ///
  hw = DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  num_channels = hw.num_channels;
  float sample_rate = DAISY.get_samplerate();

  //envelope setup
  adenv.Init(sample_rate);
  adenv.SetTime(ADENV_SEG_ATTACK, 0.05);
  adenv.SetTime(ADENV_SEG_DECAY, 2.00);
  adenv.SetMin(0);
  adenv.SetMax(1.0);
  adenv.SetCurve(-100); // - 50 does plinky sounds!!!

  adenvnoise.Init(sample_rate);
  adenvnoise.SetTime(ADENV_SEG_ATTACK, 0.05);
  adenvnoise.SetTime(ADENV_SEG_DECAY, 2.00);
  adenvnoise.SetMin(0);
  adenvnoise.SetMax(1.0);
  adenvnoise.SetCurve(-100); // - 50 does plinky sounds!!!

  // oscillator setup
  oscillator01.Init(sample_rate);
  oscillator01.SetFreq(80);
  oscillator01.SetWaveform(oscillator01.WAVE_SIN);

  //reverb setup
  verb.Init(sample_rate);
  verb.SetFeedback(0.55f);
  verb.SetLpFreq(18000.0f);
  
  //noise setup
  nse.Init();

  //filter on noise setup
  filter.Init(sample_rate);
  filter.SetFreq(1000.0f); // Set initial cutoff frequency (e.g., 1000 Hz)
  filter.SetRes(0.7f);     // Set initial resonance (e.g., 0.7)

  //Daisy setup
  DAISY.begin(ProcessAudio);

  //smoothing
  // initialize all the readings to 0:
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }
}

//processing audio
void ProcessAudio(float **in, float **out, size_t size) {

  if (peeps == 1) {
    //trigger env
    adenv.Trigger();
  }

  for (size_t i = 0; i < size; i++) {
    float out1;
    peeps = adenv.Process();
    adenv.SetCurve(plinkyness);

    float whiteNoise = nse.Process() * noiseVolume;
    oscillator01.SetAmp(1.0 * peeps);
    oscillator01.SetFreq(mappedValue);

    verb.SetFeedback(constrain(verbValue, 0.0, 0.99));
    verb.Process(oscillator01.Process(), &out1);
    filter.Process(whiteNoise);
    //peeps = adenv.Process();
    // out[0][i] = (out1 * 0.35) + (oscillator01.Process() * 0.3) + (whiteNoise * 0.2);
       out[0][i] = (out1 * 0.35) + (oscillator01.Process() * 0.3) + (filter.Band());
 
  }
}

//start controls loop

void loop() {
   if (analogRead(A2) < 930) {
    peeps = 1;
   }
   else {
    peeps = 0;
   }
  //  peeps = 1;
  // peeps = digitalRead(sensor);  //turning off the sensor for live version

  //set up smoothing
    // subtract the last reading:
  total = total - readings[readIndex];
  // read from the sensor:
  readings[readIndex] = analogRead(inputPin);
  // add the reading to the total:
  total = total + readings[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;

  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }

  // calculate the average:
  average = total / numReadings;
  // send it to the computer as ASCII digits
  // Serial.println(average);

  mappedValue = abs(map(analogRead(A2), 98, 920, 1000, 60));// change to average if you want smoothing
  // mappedValue = abs(map(average, 98, 950, 1200, 100));// change to average if you want smoothing

  plinkyness = map(analogRead(A1),250, 950, 100, -80);
  

  //reverb effect
  // verbValue = (map(analogRead(A3), 150, 950, 750, 0) * 0.001); // this maths is to get a good range from those numbers.
  verbValue = 0.3;

  //noise
  noiseMap = map(analogRead(A3), 98, 900, 100, 0)/100.0f;
  noiseVolume = constrain(noiseMap, 0.00, 90.00);
    // Use pot2Value to also adjust filter cutoff for noise2 (creates different noise colors)
   filterCutoff = noiseVolume; // Reuse the same value
   filter.SetFreq(filterCutoff * 5000.0f); // Scale to a reasonable frequency range
  
  //set up strings
  Serial.println(analogRead(A1));
  Serial.println(noiseVolume);
  // Serial.print("\t");
  // Serial.println(mappedValue);
  // Serial.println(peeps);

   delay(random(100, 150)); // not sure I want this - does it delay- steps
  //  delay(20);
}
