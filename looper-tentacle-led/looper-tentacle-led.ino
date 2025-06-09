
#include "simple-daisy.h"
#include "looper.h"

// Setup pins
static const int record_pin      = D(S33);
static const int loop_start_pin  = A(S31);
static const int loop_length_pin = A(S32);
static const int pitch_pin       = A(S30);
static const int rec_led_pin     = D(S45);  //d30
static const int play_led_pin     = D(S44);  //d29

int normalPitchIn;
int swapLength;
int constrainLength;
int normalStartIn;

static const float kKnobMax = 1023;
static const float kTentMax = 50;
static const float kTentLengthMax = 120;


// Allocate buffer in SDRAM 
static const uint32_t kBufferLengthSec = 5;
static const uint32_t kSampleRate = 48000;
static const size_t kBufferLenghtSamples = kBufferLengthSec * kSampleRate;
static float DSY_SDRAM_BSS buffer[kBufferLenghtSamples];

static synthux::Looper looper;
static PitchShifter pitch_shifter;

void AudioCallback(float **in, float **out, size_t size) {
  for (size_t i = 0; i < size; i++) {
    auto looper_out = looper.Process(in[1][i]);
    out[0][i] = out[1][i] = pitch_shifter.Process(looper_out);
  }
}

void setup() {
  Serial.begin(9600);
  DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  float sample_rate = DAISY.get_samplerate();

  // Setup looper
  looper.Init(buffer, kBufferLenghtSamples);

  // Setup pitch shifter
  pitch_shifter.Init(sample_rate);

  // Setup pins
  pinMode(record_pin, INPUT_PULLDOWN);
  pinMode(rec_led_pin, OUTPUT);
  pinMode(play_led_pin, OUTPUT);

  DAISY.begin(AudioCallback);
}

void loop() {
  // Set loop parameters
  //loopstart
  auto loop_start = fmap(analogRead(loop_start_pin) / kKnobMax, 0.f, 1.f);
  //loop length
  // normalLengthIn = constrain(map(analogRead(loop_length_pin), 3, 120, 1, 0), 0, 1);



  auto loop_length = fmap(constrainLength / kTentLengthMax, 0.001f, 1.f);
  swapLength = map(analogRead(loop_length_pin), kTentLengthMax, 0, 0, kTentLengthMax);
  constrainLength = constrain(swapLength, 0, kTentLengthMax);
  looper.SetLoop(loop_start, loop_length);

  // Toggle record
  auto record_on = digitalRead(record_pin);
  looper.SetRecording(record_on);
  if (record_on){
     digitalWrite(rec_led_pin, HIGH);
     digitalWrite(play_led_pin, LOW);
     Serial.println('record on');
  }
  else{
    digitalWrite(rec_led_pin, LOW);
    digitalWrite(play_led_pin, HIGH);
    Serial.println("record off");
  }


  // Set pitch

  //todo constrain this value
  //add pots to the circuit to set range
  auto pitch_val = fmap(analogRead(pitch_pin) / kTentMax, 0.f, 1.f);
  set_pitch(pitch_val);
  Serial.println(loop_length);
    // Serial.println(analogRead(loop_length_pin));
}

void set_pitch(float pitch_val) {
  int pitch = 0;
  // Allow some gap in the middle of the knob turn so 
  // it's easy to cacth zero position
  if (pitch_val < 0.45 || pitch_val > 0.55) {
    pitch = 12.0 * (pitch_val - 0.5);
  }
  pitch_shifter.SetTransposition(pitch);
}