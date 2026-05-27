// =========================
// ピン定義
// =========================
const int PIN_TRIG = 7;   // 超音波センサー：トリガー
const int PIN_ECHO = 8;   // 超音波センサー：エコー

const int PIN_LED_R = 6;  // RGB LED（赤）PWM
const int PIN_LED_G = 5;  // RGB LED（緑）PWM
const int PIN_LED_B = 3;  // RGB LED（青）PWM

const int PIN_BUTTON = 2; // モード切り替えボタン（プルアップ入力）

// =========================
// グローバル変数
// =========================
unsigned long lastMillisMeasure = 0; // 距離測定のタイマー
unsigned long blinkMillis = 0;       // 点滅制御用タイマー

int distance = 0; // 現在の距離（cm）

bool isBlinkMode = false; // 点滅モードかどうか
bool ledState = false;    // 点滅 ON/OFF 状態

int blinkInterval = 500;  // 点滅間隔（ms）

// LED の RGB 値
int ledR = 0;
int ledG = 0;
int ledB = 0;

const int MEASURE_INTERVAL = 100; // 距離測定間隔（ms）

// =========================
// ボタン用（デバウンス）
// =========================
bool buttonState = LOW;
bool lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 30; // チャタリング除去時間

// =========================
// モード管理
// =========================
enum Mode {
  MODE_DISTANCE, // 距離に応じて色が変わるモード
  MODE_LIGHT     // 手かざしライトモード
};
Mode currentMode = MODE_DISTANCE;

// =========================
// 通常ライトモード用
// =========================
bool whiteLightState = false; // 白色ライトの ON/OFF
bool handDetectedLast = false; // 手が近い状態の保持

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

  pinMode(PIN_BUTTON, INPUT_PULLUP); // ボタンはプルアップで使用

  digitalWrite(PIN_TRIG, LOW);
  delay(200);
}

// =========================
// loop()
// =========================
void loop() {

  // --- ボタンチェック（押されたらモード切り替え） ---
  if (readButton()) {
    toggleMode();
  }

  // --- モードごとの処理 ---
  if (currentMode == MODE_DISTANCE) {
    doMeasure();       // 距離測定
    doColorChange();   // 距離に応じた色決定
    doBlinkControl();  // 点滅設定
    updateLED();       // LED 出力
  }
  else if (currentMode == MODE_LIGHT) {
    doLightMode();     // 手かざしライトモード
  }
}

// =========================
// ボタン読み取り（デバウンス付き）
// =========================
bool readButton() {
  bool reading = digitalRead(PIN_BUTTON);
  unsigned long now = millis();

  // 状態が変わったらデバウンス開始
  if (reading != lastButtonState) {
    lastDebounceTime = now;
  }

  // 一定時間経過後に確定
  if ((now - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != buttonState) {
      buttonState = reading;

      // LOW = 押された瞬間
      if (buttonState == LOW) {
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

  // モード切り替え時は LED を消灯
  analogWrite(PIN_LED_R, 0);
  analogWrite(PIN_LED_G, 0);
  analogWrite(PIN_LED_B, 0);

  Serial.print("Mode changed: ");
  Serial.println(currentMode == MODE_DISTANCE ? "Distance" : "Light");
}

// =========================
// 距離測定（超音波センサー）
// =========================
int readDistance() {
  // トリガー信号
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // エコー時間を取得（最大 30ms）
  long duration = pulseIn(PIN_ECHO, HIGH, 30000);

  if (duration == 0) return distance; // 測定失敗時は前回値を返す

  // 距離計算（cm）
  int dist = duration * 0.0343 / 2;

  // 異常値は無視
  if (dist <= 0 || dist > 400) return distance;

  return dist;
}

// 測定タイミング管理
void doMeasure() {
  unsigned long now = millis();
  if (now - lastMillisMeasure >= MEASURE_INTERVAL) {
    distance = readDistance();
    lastMillisMeasure = now;

    Serial.print("distance = ");
    Serial.print(distance);
    Serial.println(" cm");
  }
}

// =========================
// 距離に応じた色決定
// =========================
void doColorChange() {
  isBlinkMode = false; // 初期状態は点滅なし

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
    // 5cm以下 → 赤点滅
    isBlinkMode = true;
    ledR = 255; ledG = 0; ledB = 0;
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
    // 点滅タイミング
    if (now - blinkMillis >= blinkInterval) {
      ledState = !ledState;
      blinkMillis = now;
    }

    // 点滅 ON/OFF
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
    // 点滅なし → 常時点灯
    analogWrite(PIN_LED_R, ledR);
    analogWrite(PIN_LED_G, ledG);
    analogWrite(PIN_LED_B, ledB);
  }
}

// =========================
// 手かざしライトモード
// =========================
void doLightMode() {
  int d = readDistance();
  unsigned long now = millis();

  static unsigned long nearStartTime = 0;

  const int NEAR_THRESHOLD = 5;   // 5cm以内で手を検知
  const int HOLD_ON  = 1000;      // 消灯 → 点灯：1秒
  const int HOLD_OFF = 1000;      // 点灯 → 消灯：1秒

  bool isNear = (d > 0 && d <= NEAR_THRESHOLD);

  Serial.print("LightMode distance = ");
  Serial.println(d);

  // 手が近づいた瞬間
  if (isNear && !handDetectedLast) {
    nearStartTime = now;
    handDetectedLast = true;
  }

  // 手が近い状態が続いた時間
  unsigned long holdTime = now - nearStartTime;

  // 消灯 → 点灯
  if (!whiteLightState && handDetectedLast && holdTime >= HOLD_ON) {
    whiteLightState = true;
    handDetectedLast = false;
    delay(300);  // 誤作動防止
  }

  // 点灯 → 消灯
  if (whiteLightState && handDetectedLast && holdTime >= HOLD_OFF) {
    whiteLightState = false;
    handDetectedLast = false;
    delay(300);
  }

  // 手を離したらリセット
  if (!isNear) {
    handDetectedLast = false;
  }

  // LED 出力（白色ライト）
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
