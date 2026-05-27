// ==============================================================================
// 1. ピン配置および定数の定義 (Pin assignment and Constants)
// ==============================================================================
const int PIN_TRIG     = 12; // 超音波センサー Trigピン (OUTPUT)
const int PIN_ECHO     = 13; // 超音波センサー Echoピン (INPUT)
const int PIN_MOTOR_A  = 2;  // ステッピングモーター 相A (OUTPUT)
const int PIN_MOTOR_B  = 3;  // ステッピングモーター 相B (OUTPUT)
const int PIN_MOTOR_C  = 4;  // ステッピングモーター 相C (OUTPUT)
const int PIN_MOTOR_D  = 5;  // ステッピングモーター 相D (OUTPUT)
const int PIN_BUZZER   = 6;  // アクティブブザー (OUTPUT)
// モーターのパルス制御用インターバル（ミリ秒）
const unsigned long INTERVAL_NORMAL = 2; // 巡航状態時のパルス間隔 (2ms)
const unsigned long INTERVAL_SLOW   = 5; // 減速状態時のパルス間隔 (5ms)
// ==============================================================================
// 2. グローバル変数の定義 (Global Variables - 合計 9 バイト)
// ==============================================================================
int currentState = 0;         // 走行状態管理 (0:巡航, 1:減速, 2:緊急停止)
int sensorValue  = 0;         // 最新の計測距離を格納 (cm単位)
unsigned long lastMillis = 0; // モーターのノンブロッキング制御用タイムスタンプ
int motorStep    = 0;         // モーターの励磁ステップ位置記録用 (0〜3)
// ==============================================================================
// 3. 初期化処理 (Setup)
// ==============================================================================
void setup() {
  // 各ピンの入出力モード設定
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_MOTOR_A, OUTPUT);
  pinMode(PIN_MOTOR_B, OUTPUT);
  pinMode(PIN_MOTOR_C, OUTPUT);
  pinMode(PIN_MOTOR_D, OUTPUT);
  // 初期出力状態の設定
  digitalWrite(PIN_BUZZER, LOW); // 初期状態は消音
  
  // デバッグ用シリアル通信の開始
  Serial.begin(9600);
  Serial.println("System Initialized.");
}
// ==============================================================================
// 4. メインループ (Main Loop)
// ==============================================================================
void loop() {
  readDistance();   // 1. 入力：距離データの計測
  updateState();    // 2. 判定：計測値に基づく状態遷移の更新
  controlMotor();   // 3. 出力：状態に応じたモーター駆動（調速・停止）
  controlBuzzer();  // 4. 出力：状態に応じたブザー鳴動制御
}
// ==============================================================================
// 5. 各個別関数の実装 (Function Details)
// ==============================================================================
/**
 * 超音波センサーを制御し、前方障害物との距離を算出する関数
 */
void readDistance() {
  // 超音波パルスの送信 (Trigピンの制御)
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  
  // 反射波の受信時間計測（タイムアウト30ms設定でカチ死を防止）
  long duration = pulseIn(PIN_ECHO, HIGH, 30000);
  int rawDistance = duration / 58;
  // 異常系処理：ノイズ等による仕様範囲外（0cmや400cm以上）の値をフィルタリング
  if (rawDistance > 0 && rawDistance < 400) {
    sensorValue = rawDistance; // 正常値のみ更新
  }
  
  // デバッグ出力：シリアルモニタへの描画負荷を下げるため200ms間隔で出力
  static unsigned long lastLog = 0;
  if (millis() - lastLog > 200) {
    Serial.print("Distance: "); Serial.print(sensorValue); Serial.println(" cm");
    lastLog = millis();
  }
}
/**
 * 計測距離をもとに現在の走行状態を更新する関数
 */
void updateState() {
  // 安全要件：一度「緊急停止(状態2)」に遷移した後はロックし、自動復帰させない
  if (currentState == 2) {
    return; 
  }
  // 距離に応じた状態遷移判定（15cmの境界付近でのハンチング防止バッファを考慮）
  if (sensorValue <= 5) {
    currentState = 2; // 緊急停止状態へ移行
    Serial.println("STATE CHANGED: 2 (EMERGENCY STOP)");
  } 
  else if (sensorValue <= 15) {
    currentState = 1; // 減速状態へ移行
  } 
  else if (sensorValue > 16) { 
    // 境界値でのガタつき（ハンチング）を防ぐため、巡航への復帰は16cm超とする
    currentState = 0; // 巡航状態へ移行
  }
}
/**
 * 状態に応じてステッピングモーターの速度をパルス幅で非ブロッキング制御する関数
 */
void controlMotor() {
  // 緊急停止状態の場合：全ピンを即座にLOWにしてモーターへの通電を遮断
  if (currentState == 2) {
    digitalWrite(PIN_MOTOR_A, LOW);
    digitalWrite(PIN_MOTOR_B, LOW);
    digitalWrite(PIN_MOTOR_C, LOW);
    digitalWrite(PIN_MOTOR_D, LOW);
    return;
  }
  // 現在の状態から目標とするパルス間隔（速度）を選択
  unsigned long targetInterval = (currentState == 0) ? INTERVAL_NORMAL : INTERVAL_SLOW;
  unsigned long now = millis();
  // millis()を使用した時間差分チェック（delayを使用しないノンブロッキング処理）
  if (now - lastMillis >= targetInterval) {
    lastMillis = now;
    
    // 4相1相励磁（4ステップ周期）によるステップ進行
    motorStep++;
    if (motorStep > 3) motorStep = 0;
    switch (motorStep) {
      case 0: digitalWrite(PIN_MOTOR_A, HIGH); digitalWrite(PIN_MOTOR_B, LOW);  digitalWrite(PIN_MOTOR_C, LOW);  digitalWrite(PIN_MOTOR_D, LOW);  break;
      case 1: digitalWrite(PIN_MOTOR_A, LOW);  digitalWrite(PIN_MOTOR_B, HIGH); digitalWrite(PIN_MOTOR_C, LOW);  digitalWrite(PIN_MOTOR_D, LOW);  break;
      case 2: digitalWrite(PIN_MOTOR_A, LOW);  digitalWrite(PIN_MOTOR_B, LOW);  digitalWrite(PIN_MOTOR_C, HIGH); digitalWrite(PIN_MOTOR_D, LOW);  break;
      case 3: digitalWrite(PIN_MOTOR_A, LOW);  digitalWrite(PIN_MOTOR_B, LOW);  digitalWrite(PIN_MOTOR_C, LOW);  digitalWrite(PIN_MOTOR_D, HIGH); break;
    }
  }
}
/**
 * 緊急停止状態のときのみ警告ブザーを鳴動させる関数
 */
void controlBuzzer() {
  if (currentState == 2) {
    digitalWrite(PIN_BUZZER, HIGH); // 警告音の鳴動（遅延なし）
  } else {
    digitalWrite(PIN_BUZZER, LOW);  // 消音状態を維持
  }
}