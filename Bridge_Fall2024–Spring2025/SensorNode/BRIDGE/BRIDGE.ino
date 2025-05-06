#include <Arduino.h>
#include <SPI.h>
#include <lmic.h>
#include <hal/hal.h>
#include <Adafruit_ISM330DHCX.h>
#include <TensorFlowLite_ESP32.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_error_reporter.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include "autoencoder_model.h"

// ─── OTAA (Over-The-Air Activation) Keys ─────────────────────────────────────────
// These keys are used to authenticate and join the LoRaWAN network
static const u1_t PROGMEM APPEUI[8]  = {0x78,0xDE,0xFE,0x95,0x46,0x14,0xA8,0x68};
static const u1_t PROGMEM DEVEUI[8]  = {0x2B,0x09,0x17,0x87,0x97,0xC7,0x3D,0x9A};
static const u1_t PROGMEM APPKEY[16] = {
  0xB8,0x46,0x92,0x72,0x71,0xC1,0x00,0xBE,
  0x94,0x32,0xC9,0xC2,0xA3,0xF7,0x5C,0x0F
};

// Callback functions to provide the OTAA keys to the LMIC library
void os_getArtEui(u1_t* buf){ memcpy_P(buf, APPEUI, 8); }
void os_getDevEui(u1_t* buf){ memcpy_P(buf, DEVEUI, 8); }
void os_getDevKey(u1_t* buf){ memcpy_P(buf, APPKEY,16); }

// ─── Pin Mapping for LMIC (LoRa) ───────────────────────────────────────────────
// Defines which pins of the ESP32 are connected to the LoRa module
const lmic_pinmap lmic_pins = {
  .nss  = 33,               // Chip select pin for SPI
  .rxtx = LMIC_UNUSED_PIN, // Not used (RX/TX switching)
  .rst  = 15,             // Reset pin
  .dio  = {27, 14}       // DIO0 and DIO1 pins for interrupts
};

// ─── Global Variables ──────────────────────────────────────────────────────────
volatile bool hasJoined = false;  // Flag set when the device has joined the network
Adafruit_ISM330DHCX accel;        // Accelerometer object
static osjob_t sendjob;           // LMIC job structure for scheduling sends

// Sampling parameters and buffers
const unsigned long SAMPLE_INTERVAL_US = 10000;          // 10 ms → 100 Hz sampling rate
unsigned long lastSampleTime = 0;
float raw_x[100], raw_y[100], raw_z[100];                // Raw acceleration data
float denoised_x[100], denoised_y[100], denoised_z[100]; // Denoised data after autoencoder
int buf_index = 0;                                       // Current index in the sample buffer

// TensorFlow Lite for Microcontrollers globals
tflite::MicroErrorReporter micro_error_reporter;
tflite::ErrorReporter*      error_reporter = nullptr;
const tflite::Model*        model          = nullptr;
tflite::MicroInterpreter*   interpreter    = nullptr;
TfLiteTensor*               tensor_in      = nullptr;
TfLiteTensor*               tensor_out     = nullptr;
constexpr int               kArenaSize = 16 * 1024;       // Memory arena for TFLite
static uint8_t              tensor_arena[kArenaSize];     // TFLite tensor memory

// ─── Function Declarations ─────────────────────────────────────────────────────
void onEvent(ev_t ev);            // LMIC event callback
void setupAccelerometer();        // Initialize accelerometer settings
void TfliteSetup();               // Load and prepare the TFLite model
void doInference(const float* src, float* dst); // Run one inference on a data buffer
void sendFirstXYZ();              // Package and send the first sample via LoRa

// ─── LMIC Event Handler ────────────────────────────────────────────────────────
void onEvent(ev_t ev) {
  Serial.print(os_getTime()); Serial.print(": ");
  switch (ev) {
    case EV_JOINING:    Serial.println(F("EV_JOINING"));    break;
    case EV_JOINED:
      Serial.println(F("EV_JOINED"));
      hasJoined = true;                // Mark network join complete
      LMIC_setLinkCheckMode(0);        // Disable link check to save power
      break;
    case EV_TXSTART:    Serial.println(F("EV_TXSTART"));    break;
    case EV_TXCOMPLETE: Serial.println(F("EV_TXCOMPLETE")); break;
    default:            Serial.println(F("EV_OTHER"));      break;
  }
}

// ─── Setup Function ───────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  while(!Serial) delay(10);          // Wait for Serial to be ready
  Serial.println(F("=== Init ==="));

  setupAccelerometer();              // Initialize accelerometer hardware
  TfliteSetup();                     // Initialize TFLite interpreter and model

  randomSeed(esp_random());          // Seed random generator for LMIC
  os_init();                         // Initialize LMIC runtime
  LMIC_reset();                      // Reset LoRa state
  LMIC_startJoining();               // Begin network join procedure

  buf_index = 0;                     // Reset sample buffer index
  lastSampleTime = micros();         // Initialize timer for sampling
}

// ─── Main Loop ─────────────────────────────────────────────────────────────────
void loop() {
  unsigned long now = micros();
  // Check if it's time to sample again
  if (now - lastSampleTime >= SAMPLE_INTERVAL_US) {
    lastSampleTime = now;

    // 1) Read accelerometer data
    sensors_event_t a, g, t;
    accel.getEvent(&a, &g, &t);
    raw_x[buf_index] = a.acceleration.x;
    raw_y[buf_index] = a.acceleration.y;
    raw_z[buf_index] = a.acceleration.z;
    buf_index++;

    // 2) After collecting 100 samples, run denoising inference
    if (buf_index >= 100) {
      buf_index = 0;
      Serial.println(F("→ Running TFLite inference on X, Y, Z…"));

      // Run autoencoder on each axis data
      doInference(raw_x, denoised_x);
      doInference(raw_y, denoised_y);
      doInference(raw_z, denoised_z);

      // Print comparison of first sample before/after denoising
      Serial.print(F("Raw[0]  X,Y,Z: "));
      Serial.print(raw_x[0],3); Serial.print(F(", "));
      Serial.print(raw_y[0],3); Serial.print(F(", "));
      Serial.println(raw_z[0],3);

      Serial.print(F("Denoised[0] X,Y,Z: "));
      Serial.print(denoised_x[0],3); Serial.print(F(", "));
      Serial.print(denoised_y[0],3); Serial.print(F(", "));
      Serial.println(denoised_z[0],3);

      // 3) Send the first denoised sample if joined
      if (hasJoined) {
        sendFirstXYZ();
      } else {
        Serial.println(F("Not joined yet, skipping send"));
      }
    }
  }

  // Service LoRa events (transmit/reception)
  os_runloop_once();
}

// ─── Quantize, Invoke, and Dequantize Inference ────────────────────────────────
void doInference(const float* src, float* dst) {
  // Retrieve quantization parameters
  float in_scale  = tensor_in->params.scale;
  int   in_zp     = tensor_in->params.zero_point;
  float out_scale = tensor_out->params.scale;
  int   out_zp    = tensor_out->params.zero_point;

  // Quantize input floats to int8
  for (int i = 0; i < 100; i++) {
    int32_t q = lround(src[i] / in_scale + in_zp);
    q = q < -128 ? -128 : (q > 127 ? 127 : q);
    tensor_in->data.int8[i] = (int8_t)q;
  }

  // Run the model
  if (interpreter->Invoke() != kTfLiteOk) {
    Serial.println(F("!!! Inference failed"));
    memset(dst, 0, 100 * sizeof(float)); // Zero output on failure
    return;
  }

  // Dequantize the output back to float
  for (int i = 0; i < 100; i++) {
    int8_t qout = tensor_out->data.int8[i];
    dst[i] = (qout - out_zp) * out_scale;
  }
}

// ─── Send First Denoised Sample via LoRa ────────────────────────────────────────
void sendFirstXYZ() {
  float x = denoised_x[0], y = denoised_y[0], z = denoised_z[0];

  Serial.print(F("→ Sending denoised X,Y,Z: "));
  Serial.print(x,3); Serial.print(F(", "));
  Serial.print(y,3); Serial.print(F(", "));
  Serial.println(z,3);

  // Pack floats into a byte payload
  uint8_t payload[3 * sizeof(float)];
  memcpy(payload + 0, &x, sizeof(float));
  memcpy(payload + 4, &y, sizeof(float));
  memcpy(payload + 8, &z, sizeof(float));

  // Print payload in hex for debugging
  Serial.print(F("Payload bytes: "));
  for (int i = 0; i < sizeof(payload); i++) {
    if (payload[i] < 0x10) Serial.print('0');
    Serial.print(payload[i], HEX);
    Serial.print(' ');
  }
  Serial.println();

  // Transmit if no TX/RX pending
  if (!(LMIC.opmode & OP_TXRXPEND)) {
    LMIC_setTxData2(1, payload, sizeof(payload), 0);
    Serial.println(F("Packet queued"));
  } else {
    Serial.println(F("TX pending, skipping"));
  }
}

// ─── Initialize the ISM330DHCX Accelerometer ───────────────────────────────────
void setupAccelerometer() {
  Serial.println(F("Init ISM330DHCX…"));
  if (!accel.begin_I2C()) {
    Serial.println(F("Accel not found!"));
    while (1) delay(10); // Halt if sensor is missing
  }
  accel.setAccelRange(LSM6DS_ACCEL_RANGE_2_G);  // ±2g range
  accel.setAccelDataRate(LSM6DS_RATE_104_HZ);   // ~104 Hz output data rate
  Serial.println(F("Accel 104Hz"));
}

// ─── Load and Prepare the TensorFlow Lite Model ─────────────────────────────────
void TfliteSetup() {
  Serial.println(F("Loading TFLite model…"));
  error_reporter = &micro_error_reporter;
  model = tflite::GetModel(autoencoder_model_INT8_tflite);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    error_reporter->Report("Schema mismatch");
    while (1); // Halt on schema error
  }
  static tflite::MicroInterpreter static_interpreter(
    model, tflite::AllOpsResolver(), tensor_arena, kArenaSize, error_reporter
  );
  interpreter = &static_interpreter;
  interpreter->AllocateTensors();                 // Reserve memory
  tensor_in  = interpreter->input(0);            // Input tensor handle
  tensor_out = interpreter->output(0);           // Output tensor handle
  Serial.print(F("Input tensor type="));  Serial.println(tensor_in->type);
  Serial.print(F("Output tensor type=")); Serial.println(tensor_out->type);
  Serial.println(F("TFLite ready"));
}
