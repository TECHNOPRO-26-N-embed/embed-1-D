// ---- ピン設定 ----
const int trigPin = 9;
const int echoPin = 10;
const int ledPin  = 3;
const int motorPin = 5;

// ---- 定数 ----
const int SENSOR_INTERVAL = 100;  // 100ms周期
const int THRESHOLD = 50;         // 50cm以内で動作

// ---- 変数 ----
int currentState = 0;   // 0:待機, 1:動作
int distance = 0;
unsigned long lastSensorMillis = 0;

// ===============================
// 初期化
// ===============================
void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(motorPin, OUTPUT);

  Serial.begin(9600);
  digitalWrite(ledPin, LOW);
  digitalWrite(motorPin, LOW);
}

// ===============================
// メインループ
// ===============================
void loop() {
  unsigned long now = millis();

  // センサー読み取り（100ms周期）
  if (now - lastSensorMillis >= SENSOR_INTERVAL) {
    lastSensorMillis = now;
    distance = readSensor();
    Serial.print("Distance: ");
    Serial.println(distance);
  }

  // 状態更新
  updateState();

  // 出力制御
  updateOutput();
}

// ===============================
// 超音波センサー読み取り
// ===============================
int readSensor() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  int dist = duration * 0.034 / 2;

  // 異常値処理（0cm / 400cm以上は無視）
  if (dist <= 0 || dist >= 400) {
    return distance;  // 前回値を返す
  }

  return dist;
}

// ===============================
// 状態遷移
// ===============================
void updateState() {
  if (distance <= THRESHOLD) {
    currentState = 1;  // 動作状態
  } else {
    currentState = 0;  // 待機状態
  }
}

// ===============================
// 出力制御
// ===============================
void updateOutput() {
  if (currentState == 1) {
    turnOnLED();
    motorOn();
  } else {
    turnOffLED();
    motorOff();
  }
}

// ===============================
// LED ON
// ===============================
void turnOnLED() {
  digitalWrite(ledPin, HIGH);
}

// ===============================
// LED OFF
// ===============================
void turnOffLED() {
  digitalWrite(ledPin, LOW);
}

// ===============================
// モーター ON
// ===============================
void motorOn() {
  digitalWrite(motorPin, HIGH);
}

// ===============================
// モーター OFF
// ===============================
void motorOff() {
  digitalWrite(motorPin, LOW);
}



