const int trigPin = 9;
const int echoPin = 10;
const int ledPin  = 8;
const int en1Pin  = 5;  // EN1 → D5
const int in1Pin  = 4;  // IN1 → D4
const int in2Pin  = 3;  // IN2 → D3

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
  pinMode(en1Pin, OUTPUT);
  pinMode(in1Pin, OUTPUT);
  pinMode(in2Pin, OUTPUT);

  Serial.begin(9600);
  digitalWrite(ledPin, LOW);
  motoroff();
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
    motoroff();
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
  digitalWrite(en1Pin, HIGH);  // 有効化
  digitalWrite(in1Pin, HIGH);  // IN1: HIGH
  digitalWrite(in2Pin, LOW);   // IN2: LOW
}



// ===============================
// モーター OFF
// ===============================
void motoroff() {
  digitalWrite(en1Pin, LOW);   // 無効化
  digitalWrite(in1Pin, LOW);
  digitalWrite(in2Pin, LOW);
}



