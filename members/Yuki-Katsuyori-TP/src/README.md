# ソースコードフォルダ

実習のソースコード（.c / .ino ファイル等）をここに置いてください。

const int PIN_TRIG = 7;
const int PIN_ECHO = 8;
const int PIN_BUZZER = 9;

void setup() {
  Serial.begin(9600);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_TRIG, LOW);
}

void loop() {

  // --- 距離測定 ---
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long duration = pulseIn(PIN_ECHO, HIGH, 30000);
  float distance = duration * 0.034 / 2;

  if (duration == 0 || distance <= 0 || distance > 400) {
    noTone(PIN_BUZZER);
    return;
  }

  Serial.print("Distance: ");
  Serial.println(distance);

  // --- 3段階の警告音 ---
  if (distance <= 15) {
    // 危険：連続警告音
    tone(PIN_BUZZER, 3000);  // 高音
    delay(50);

  } else if (distance <= 30) {
    // 警告：速いピピピ
    tone(PIN_BUZZER, 2500, 80);
    delay(120);

  } else if (distance <= 50) {
    // 注意：ゆっくりピッ…ピッ…
    tone(PIN_BUZZER, 1500, 80);
    delay(300);

  } else {
    // 安全：無音
    noTone(PIN_BUZZER);
  }
}

## ファイル例

```
unit1_hello.c
unit2_led.ino
```
