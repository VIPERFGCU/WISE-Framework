#include <Arduino.h>
#include <Adafruit_ISM330DHCX.h>
#include <TensorFlowLite_ESP32.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_error_reporter.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include "autoencoder_model.h"

// ─── Accelerometer & Sampling ──────────────────────────────────────────────────
Adafruit_ISM330DHCX accel;
const unsigned long SAMPLE_INTERVAL_US = 10000;  // 100 Hz
unsigned long lastSampleTime = 0;

float raw_x[100];
float denoised_x[100];
int buf_index = 0;

// ─── TFLite Globals ─────────────────────────────────────────────────────────────
tflite::MicroErrorReporter micro_error_reporter;
tflite::ErrorReporter*      error_reporter = nullptr;
const tflite::Model*        model          = nullptr;
tflite::MicroInterpreter*   interpreter    = nullptr;
TfLiteTensor*               tensor_in      = nullptr;
TfLiteTensor*               tensor_out     = nullptr;
constexpr int               kArenaSize = 16 * 1024;
static uint8_t              tensor_arena[kArenaSize];

// ─── Function Declarations ─────────────────────────────────────────────────────
void setupAccelerometer();
void TfliteSetup();
void doInference(const float* src, float* dst);
float compute_rms(const float* a, int N);
float compute_rmse(const float* a, const float* b, int N);
float compute_mse(const float* a, const float* b, int N);

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println(F("=== Init ==="));

  setupAccelerometer();
  TfliteSetup();

  buf_index = 0;
  lastSampleTime = micros();
}

void loop() {
  unsigned long now = micros();
  if (now - lastSampleTime >= SAMPLE_INTERVAL_US) {
    lastSampleTime += SAMPLE_INTERVAL_US;
    // 1) Read X
    sensors_event_t a, g, t;
    accel.getEvent(&a, &g, &t);
    raw_x[buf_index++] = a.acceleration.x;

    // 2) Once 100, denoise & compute metrics
    if (buf_index >= 100) {
      buf_index = 0;
      doInference(raw_x, denoised_x);

      float rms  = compute_rms(denoised_x, 100);
      float rmse = compute_rmse(raw_x, denoised_x, 100);
      float mse  = compute_mse (raw_x, denoised_x, 100);

      // Print single CSV line: raw_x[0], denoised_x[0], rms, rmse, mse
      Serial.print(raw_x[0], 3);    Serial.print(',');
      Serial.print(denoised_x[0],3); Serial.print(',');
      Serial.print(rms,  3);        Serial.print(',');
      Serial.print(rmse, 3);        Serial.print(',');
      Serial.println(mse,  3);
    }
  }
}

// ─── Accelerometer Setup ───────────────────────────────────────────────────────
void setupAccelerometer() {
  Serial.println(F("Init ISM330DHCX…"));
  if (!accel.begin_I2C()) {
    Serial.println(F("Accel not found!")); while (1) delay(10);
  }
  accel.setAccelRange(LSM6DS_ACCEL_RANGE_4_G);
  accel.setAccelDataRate(LSM6DS_RATE_104_HZ);
  Serial.println(F("Accel @ ~104Hz"));
}

// ─── TFLite Interpreter Setup ─────────────────────────────────────────────────
void TfliteSetup() {
  Serial.println(F("Loading TFLite model…"));
  error_reporter = &micro_error_reporter;
  model = tflite::GetModel(autoencoder_model_INT8_tflite);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    error_reporter->Report("Model schema mismatch"); while(1);
  }
  static tflite::MicroInterpreter static_interpreter(
    model, tflite::AllOpsResolver(), tensor_arena, kArenaSize, error_reporter
  );
  interpreter = &static_interpreter;
  interpreter->AllocateTensors();
  tensor_in  = interpreter->input(0);
  tensor_out = interpreter->output(0);
  Serial.println(F("TFLite ready"));
}

// ─── Quantize, Invoke, Dequantize ──────────────────────────────────────────────
void doInference(const float* src, float* dst) {
  float in_scale  = tensor_in->params.scale;
  int   in_zp     = tensor_in->params.zero_point;
  float out_scale = tensor_out->params.scale;
  int   out_zp    = tensor_out->params.zero_point;
  for (int i = 0; i < 100; i++) {
    int32_t q = lround(src[i] / in_scale + in_zp);
    q = q < -128 ? -128 : (q > 127 ? 127 : q);
    tensor_in->data.int8[i] = (int8_t)q;
  }
  if (interpreter->Invoke() != kTfLiteOk) {
    Serial.println(F("!!! Inference failed"));
    memset(dst, 0, 100 * sizeof(float));
    return;
  }
  for (int i = 0; i < 100; i++) {
    int8_t qout = tensor_out->data.int8[i];
    dst[i] = (qout - out_zp) * out_scale;
  }
}

// ─── Metrics ───────────────────────────────────────────────────────────────────
float compute_rms(const float* a, int N) {
  float sum2 = 0;
  for (int i = 0; i < N; i++) sum2 += a[i] * a[i];
  return sqrt(sum2 / N);
}

float compute_rmse(const float* a, const float* b, int N) {
  float sum2 = 0;
  for (int i = 0; i < N; i++) {
    float d = a[i] - b[i];
    sum2 += d * d;
  }
  return sqrt(sum2 / N);
}

float compute_mse(const float* a, const float* b, int N) {
  float sum2 = 0;
  for (int i = 0; i < N; i++) {
    float d = a[i] - b[i];
    sum2 += d * d;
  }
  return sum2 / N;
}
