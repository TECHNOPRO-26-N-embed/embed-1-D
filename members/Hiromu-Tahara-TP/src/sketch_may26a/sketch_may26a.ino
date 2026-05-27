// =========================
// ピン定義
// =========================
const int PIN_TRIG = 7;
const int PIN_ECHO = 8;

const int PIN_LED_R = 6;  // PWM
const int PIN_LED_G = 5;  // PWM
const int PIN_LED_B = 3;  // PWM

const int PIN_BUTTON = 2; // ← 追加（モード切り替えボタン）

// =========================
// グローバル変数
// =========================
unsigned long lastMillisMeasure = 0;
unsigned long blinkMillis = 0;

int distance = 0;

bool isBlinkMode = false;
bool ledState = false;

int blinkInterval = 500;

int ledR = 0;
int ledG = 0;
int ledB = 0;

const int MEASURE_INTERVAL = 100;

// =========================
// ボタン用（デバウンス）
// =========================
bool buttonState = LOW;
bool lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 30;

// =========================
// モード管理
// =========================
enum Mode {
  MODE_DISTANCE,
  MODE_LIGHT
};
Mode currentMode = MODE_DISTANCE;

// =========================
// 通常ライトモード用
// =========================
bool whiteLightState = false;
bool handDetectedLast = false;

// =========================
// setup()
// =========================
void setup() {
  Serial.begin(9600);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);

  pinMode(PIN_BUTTON, INPUT_PULLUP); // ← 追加

  digitalWrite(PIN_TRIG, LOW);
  delay(200);
}

// =========================
// loop()
// =========================
void loop() {

  // --- ボタンチェック ---
  if (readButton()) {
    toggleMode();
  }

  // --- モードごとの処理 ---
  if (currentMode == MODE_DISTANCE) {
    doMeasure();
    doColorChange();
    doBlinkControl();
    updateLED();
  }
  else if (currentMode == MODE_LIGHT) {
    doLightMode();
  }
}

// =========================
// ボタン読み取り（デバウンス）
// =========================
bool readButton() {
  bool reading = digitalRead(PIN_BUTTON);
  unsigned long now = millis();

  if (reading != lastButtonState) {
    lastDebounceTime = now;
  }

  if ((now - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {  // 押された瞬間
        lastButtonState = reading;
        return true;
      }
    }
  }

  lastButtonState = reading;
  return false;
}

// =========================
// モード切り替え
// =========================
void toggleMode() {
  if (currentMode == MODE_DISTANCE) {
    currentMode = MODE_LIGHT;
    whiteLightState = false;
    handDetectedLast = false;
  } else {
    currentMode = MODE_DISTANCE;
  }

  analogWrite(PIN_LED_R, 0);
  analogWrite(PIN_LED_G, 0);
  analogWrite(PIN_LED_B, 0);

  Serial.print("Mode changed: ");
  Serial.println(currentMode == MODE_DISTANCE ? "Distance" : "Light");
}

// =========================
// 距離測定
// =========================
int readDistance() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);

  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long duration = pulseIn(PIN_ECHO, HIGH, 30000);
  if (duration == 0) return distance;

  int dist = duration * 0.0343 / 2;
  if (dist <= 0 || dist > 400) return distance;

  return dist;
}

void doMeasure() {
  unsigned long now = millis();
  if (now - lastMillisMeasure >= MEASURE_INTERVAL) {
    int newDist = readDistance();
    distance = newDist;
    lastMillisMeasure = now;

    Serial.print("distance = ");
    Serial.print(distance);
    Serial.println(" cm");
  }
}

// =========================
// 色決定（距離反応モード）
// =========================
void doColorChange() {
  isBlinkMode = false;

  if (distance > 30) {
    ledR = 0; ledG = 0; ledB = 255;   // 青
  }
  else if (distance > 10) {
    ledR = 255; ledG = 255; ledB = 0; // 黄
  }
  else if (distance > 5) {
    ledR = 255; ledG = 0; ledB = 0;   // 赤
  }
  else {
    isBlinkMode = true;
    ledR = 255; ledG = 0; ledB = 0;   // 赤点滅
  }
}

// =========================
// 点滅制御
// =========================
void doBlinkControl() {
  blinkInterval = isBlinkMode ? 100 : 500;
}

// =========================
// LED 出力
// =========================
void updateLED() {
  unsigned long now = millis();

  if (isBlinkMode) {
    if (now - blinkMillis >= blinkInterval) {
      ledState = !ledState;
      blinkMillis = now;
    }

    if (ledState) {
      analogWrite(PIN_LED_R, ledR);
      analogWrite(PIN_LED_G, ledG);
      analogWrite(PIN_LED_B, ledB);
    } else {
      analogWrite(PIN_LED_R, 0);
      analogWrite(PIN_LED_G, 0);
      analogWrite(PIN_LED_B, 0);
    }
  }
  else {
    analogWrite(PIN_LED_R, ledR);
    analogWrite(PIN_LED_G, ledG);
    analogWrite(PIN_LED_B, ledB);
  }
}

// =========================
// 通常ライトモード
// =========================
void doLightMode() {
  int d = readDistance();
  unsigned long now = millis();

  static unsigned long nearStartTime = 0;

  const int NEAR_THRESHOLD = 5;   //  5cm以内で手を検知
  const int HOLD_ON  = 1000;        // 消灯 → 点灯：1秒
  const int HOLD_OFF = 1000;       // 点灯 → 消灯：1秒

  bool isNear = (d > 0 && d <= NEAR_THRESHOLD);

  // デバッグ用（必要なら残す）
  Serial.print("LightMode distance = ");
  Serial.println(d);

  // 手が近づいた瞬間
  if (isNear && !handDetectedLast) {
    nearStartTime = now;
    handDetectedLast = true;
  }

  // 手が近い状態が続いた時間
  unsigned long holdTime = now - nearStartTime;

  // 点灯していない → 点灯させたい（0.5秒）
  if (!whiteLightState && handDetectedLast && holdTime >= HOLD_ON) {
    whiteLightState = true;
    handDetectedLast = false;
    delay(300);  // チャタリング防止
  }

  // 点灯している → 消灯させたい（1秒）
  if (whiteLightState && handDetectedLast && holdTime >= HOLD_OFF) {
    whiteLightState = false;
    handDetectedLast = false;
    delay(300);  // チャタリング防止
  }

  // 手を離したらリセット
  if (!isNear) {
    handDetectedLast = false;
  }

  // LED 出力
  if (whiteLightState) {
    analogWrite(PIN_LED_R, 255);
    analogWrite(PIN_LED_G, 255);
    analogWrite(PIN_LED_B, 255);
  } else {
    analogWrite(PIN_LED_R, 0);
    analogWrite(PIN_LED_G, 0);
    analogWrite(PIN_LED_B, 0);
  }
}
