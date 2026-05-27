/* ==============================
   ピン定義
   ============================== */
const int PIN_TRIG   = 7;
const int PIN_ECHO   = 8;
const int PIN_BUZZER = 9;

/* ==============================
   状態管理
   ============================== */
// 0: 待機中, 1: 警告中
int currentState = 0;

/* ==============================
   タイマー管理（millis用）
   ============================== */
unsigned long lastDistanceMillis = 0;
unsigned long lastBeepMillis     = 0;

/* ==============================
   センサー値
   ============================== */
int distance = 0;
int lastValidDistance = 0;

/* ==============================
   定数（周期）
   ============================== */
const unsigned long DISTANCE_INTERVAL = 100;  // 距離測定周期 [ms]
const unsigned long BEEP_INTERVAL     = 300;  // ブザー周期 [ms]

/* ==============================
   関数プロトタイプ宣言
   ============================== */
int  readDistance();
void updateState(int dist);
void updateBuzzerAdvanced(int dist);


/* ==============================
   setup — 初期化
   ============================== */
void setup() {
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  Serial.begin(9600);

  // 起動確認音
  tone(PIN_BUZZER, 2000, 80);
}


/* ==============================
   loop — メインループ
   ============================== */
void loop() {
  unsigned long now = millis();

  // ① 距離測定（100ms周期）
  if (now - lastDistanceMillis >= DISTANCE_INTERVAL) {
    distance = readDistance();
    lastDistanceMillis = now;

    Serial.print("Distance: ");
    Serial.println(distance);
  }

  // ② 状態更新（ヒステリシス付き）
  updateState(distance);

  // ③ ブザー制御（300ms周期）
  if (now - lastBeepMillis >= BEEP_INTERVAL) {
    updateBuzzerAdvanced(distance);
    lastBeepMillis = now;
  }
}


/* ==============================
   readDistance — 超音波距離測定
   ============================== */
int readDistance() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long duration = pulseIn(PIN_ECHO, HIGH, 30000);

  if (duration == 0) {
    return lastValidDistance;
  }

  float distF = duration * 0.034 / 2.0;
  int dist = (int)distF;

  if (dist <= 0 || dist > 400) {
    return lastValidDistance;
  }

  lastValidDistance = dist;
  return dist;
}


/* ==============================
   updateState — 状態遷移（ヒステリシス）
   ============================== */
void updateState(int dist) {
  if (dist < 0) return;

  if (currentState == 0) {
    if (dist <= 50) currentState = 1;
  }
  else if (currentState == 1) {
    if (dist >= 55) currentState = 0;
  }
}


/* ==============================
   updateBuzzerAdvanced — 改良版4段階ブザー
   ============================== */
void updateBuzzerAdvanced(int dist) {

  if (currentState == 0) {
    noTone(PIN_BUZZER);
    return;
  }

  // ① HIGH（危険：20cm以下）→ 標準高速ビープ（50ms / 50ms）
  if (dist <= 20) {
    tone(PIN_BUZZER, 3000, 50);  // 50ms ON
    delay(50);                   // 50ms OFF
    noTone(PIN_BUZZER);
    Serial.println("HIGH (fast beep)");
  }
  // ② MID（注意：20〜35cm）→ 1500Hz
  else if (dist <= 35) {
    tone(PIN_BUZZER, 1500, 80);
    Serial.println("MID");
  }
  // ③ LOW（軽い注意：35〜50cm）→ 800Hz
  else if (dist <= 50) {
    tone(PIN_BUZZER, 800, 80);
    Serial.println("LOW");
  }
  // ④ SILENT（安全：50cm以上）
  else {
    noTone(PIN_BUZZER);
    Serial.println("silent");
  }
}
