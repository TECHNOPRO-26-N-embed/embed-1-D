# 詳細設計書 — 組込み開発実習

<!-- 作成者: あなたの名前 / 日付: YYYY-MM-DD / グループ: 〇-〇 -->

> **このドキュメントの目的**
> 基本設計書（basic_design.md）で「**どのような構造で作るか**」を決めました。
> この詳細設計書では「**各処理を具体的にどう実装するか**」を決めます。
> 書き終わったとき、**コードの骨格がほぼ完成している**状態を目指してください。

> [!NOTE]
> **V字モデルにおける位置づけ**
> 詳細設計書 ←→ **単体テスト**（関数・部品ごとのテスト）が対応します。
> 「この関数が正しく動くか」の確認は Section 5 の単体テスト仕様書で計画します。
> ※ 必須機能全体が動くかの「結合テスト」は基本設計書（Section 6）に記載します。

---

## 0. 基本設計書との接続確認

| 項目 | basic_design.md から転記 |
|:--|:--|
| 作品タイトル | 手を近づけると反応する距離警告ブザー|
| 状態の種類（1-2 状態遷移から） | |
[電源ON / 初期化]
        ↓（ピン設定完了）
[待機中] ──（距離 ≤ 50cm）──→ [警告中]
    ↑                                 │
    └────────（距離 > 50cm）──────────────┘

| 実装する関数の数（2-2 関数一覧から） | 　7個 |
| グローバル変数の合計バイト数（2-1 SRAM確認から） | 　14B |

---

## 1. グローバル変数・定数の設計

> ※ 基本設計書（2-1 データ設計）をもとに、**型と初期値まで**決めます。
> ここで設計した変数は、この後の関数設計でそのまま使います。

```
/* ==============================
   ピン定義
   ============================== */
const int PIN_TRIG   = 7;
const int PIN_ECHO   = 8;
const int PIN_BUZZER = 9;

/* ==============================
   状態管理
   ============================== */
int currentState = 0;   // 0:待機中, 1:警告中

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
const unsigned long DISTANCE_INTERVAL = 100;  // ms
const unsigned long BEEP_INTERVAL     = 300;  // ms

/* ==============================
   ブザー設定（duration方式）
   ============================== */
const int BEEP_FREQ     = 2000;  // Hz
const int BEEP_DURATION = 80;    // ms


---

## 2. 各関数の詳細設計

> ※ 基本設計書（2-2 関数一覧）で定義した各関数の「中身」を設計します。
> **疑似コード**（日本語＋処理の流れ）で書いてください。実際のC++コードは書かなくてOKです。

---

### `setup()` — 初期化処理

```
【処理の流れ】
1. 超音波センサー用ピンのモードを設定
   - PIN_TRIG → OUTPUT
   - PIN_ECHO → INPUT

2. ブザー用ピンのモードを設定
   - PIN_BUZZER → OUTPUT

3. 変数の初期化
   - currentState        = 0
   - distance            = 0
   - lastValidDistance   = 0
   - lastDistanceMillis  = 0
   - lastBeepMillis      = 0

4. シリアル通信開始（任意）
   - Serial.begin(9600)

5. 起動確認（任意）
   - tone(PIN_BUZZER, BEEP_FREQ, 80) を1回鳴らす

---

### `loop()` — メインループ

> ※ loop() は「状態ごとに何をするか」だけ書く。細かい処理は各関数に任せる。

```
【処理の流れ】

1. 現在時刻 now = millis() を取得

2. 距離測定タイミングか確認
   - if (now - lastDistanceMillis >= DISTANCE_INTERVAL):
       - distance = readDistance()
       - lastDistanceMillis = now

3. 状態更新
   - updateState(distance)

4. ブザー制御タイミングか確認
   - if (now - lastBeepMillis >= BEEP_INTERVAL):
       - updateBuzzer(currentState)
       - lastBeepMillis = now


＜currentState = 0（待機中）＞
- ブザーは鳴らさない（updateBuzzer 内で noTone）

＜currentState = 1（警告中）＞
- duration方式で短く鳴らす（80ms）

＜その他の値＞
- 安全側として currentState = 0 に戻す


### （関数ごとに以下のブロックをコピーして追加してください）

> ※ 基本設計書 2-2 の関数一覧に記載した関数を1つずつ設計します。

---

### `関数名()` — （役割を1行で書く）

readDistance() — 超音波距離測定

【処理の流れ】
1. Trigピンを LOW にして 2μs 待つ

2. Trigピンを HIGH にして 10μs パルスを出す

3. Trigピンを LOW に戻す

4. Echoピンが HIGH の間の時間を pulseIn() で計測
   - timeout = 30000μs（30ms）

5. 距離(cm) = 時間(μs) × 0.034 / 2 で計算

6. 異常値判定
   - 0cm または 400cm超
   - pulseIn が 0
   → 異常値として lastValidDistance を返す

7. 正常値の場合
   - lastValidDistance = distance
   - distance を返す

----

updateState() — 状態遷移（ヒステリシス付き）

【処理の流れ】
1. currentState = 0（待機中）
   - dist ≤ 50cm → currentState = 1（警告中）

2. currentState = 1（警告中）
   - dist ≥ 55cm → currentState = 0（待機中）

3. 45〜55cmの範囲では状態を変更しない

4. dist が負など明らかに異常な場合
   - 状態を変更せず currentState を維持
-----

updateBuzzer() — ブザー制御（duration方式）

【処理の流れ】
1. state = 0（待機中）
   - noTone(PIN_BUZZER)

2. state = 1（警告中）
   - tone(PIN_BUZZER, BEEP_FREQ, BEEP_DURATION)

3. その他の値
   - 安全側として noTone(PIN_BUZZER)
---

updateBuzzerAdvanced() — 任意機能（距離でテンポを変える）

【処理の流れ】
1. dist ≤ 30cm
   - tone(PIN_BUZZER, 2500, 80)  // 高速ビープ

2. 30cm < dist ≤ 50cm
   - tone(PIN_BUZZER, 2000, 80)  // 通常ビープ

3. dist > 50cm
   - noTone(PIN_BUZZER)

---

## 3. 重要ロジックの詳細設計

### 3-1. チャタリング防止（デバウンス処理）

> ※ ボタンを使う場合は必ず設計してください。

使わない

---

### 3-2. millis() を使ったタイマー管理

```
【距離測定】
- 100ms周期
- if (now - lastDistanceMillis >= 100)

【ブザー制御】
- 300ms周期
- if (now - lastBeepMillis >= 300)

---

### 3-3. その他の重要ロジック（任意）

【【ヒステリシス】
- 50cm以下 → 警告中へ
- 55cm以上 → 待機中へ
- 45〜55cmは状態を変えない

【異常値処理】
- 0cm、400cm超、pulseIn=0 → lastValidDistance を使用


---

## 4. デバッグ出力計画（任意）

| No | 確認したい内容 | 挿入する関数 | Serial.println の内容例 |
| --- | --- | --- | --- |
| 1 | 距離が正しいか | readDistance() | distance |
| 2 | 状態遷移の確認 | updateState() | currentState |
| 3 | 異常値処理 | readDistance() | lastValidDistance |
| 4 | ブザー制御周期 | updateBuzzer() | "beep" |
---

## 5. 単体テスト仕様書（V字モデル：詳細設計 ↔ 単体テスト）

> ※ 各関数・部品が「単体で正しく動くか」を確認するテスト項目を設計します。
> 「実際の結果」欄は実装後に記入します。

### 5-1. 入力系テスト

| No | テスト対象 | 入力・操作 | 期待結果 | 実際の結果 | 合否 |
| --- | --- | --- | --- | --- | --- |
| 1 | readDistance | 手を30cmに置く | 約30cmが返る |  | [ ] |
| 2 | readDistance | 手を100cmに置く | 約100cmが返る |  | [ ] |
| 3 | readDistance | 壁に向ける | lastValidDistance が返る |  | [ ] |
| 4 | readDistance | センサーを塞ぐ | lastValidDistance が返る |  | [ ] |
| 5 | readDistance | 48〜52cmで動かす | 状態は変わらない |  | [ ] |

### 5-2. 出力系テスト

| No | テスト対象 | 入力 | 期待結果 | 実際の結果 | 合否 |
| --- | --- | --- | --- | --- | --- |
| 1 | updateBuzzer | state=0 | ブザー停止 |  | [ ] |
| 2 | updateBuzzer | state=1 | 80ms鳴る |  | [ ] |
| 3 | updateBuzzer | 300ms周期で呼ぶ | 一定テンポで鳴る |  | [ ] |
| 4 | updateBuzzer | state=異常値 | ブザー停止 |  | [ ] |


### 5-3. タイミング・並行動作テスト

| No | テスト内容 | 手順 | 期待結果 | 実際の結果 | 合否 |
| --- | --- | --- | --- | --- | --- |
| 1 | 距離測定周期 | Serialで距離を確認 | 約100ms周期で更新 |  | [ ] |
| 2 | ブザー周期 | Serialで"beep"確認 | 約300ms周期で鳴る |  | [ ] |
| 3 | 並行動作 | 手を前後に動かす | 距離測定とブザーが同時に動く |  | [ ] |


---

## 6. AIレビュー記録

> グループレビューの前に必ず実施してください。

### Q1: 実装上の問題確認

> 「この詳細設計書に書いた関数と処理フローをもとに Arduino でコードを書きます。バグになりやすい箇所・処理の抜け・型の問題はありますか？」

AIの回答（要約）：

距離測定の異常値処理が適切

状態遷移条件（50cm / 55cm）が明確

millis() の周期管理（100ms / 300ms）が正しく設計されている

ブザー制御の安全側処理がある

型（unsigned long / int / bool）に問題なし

対応した内容：

異常値処理の明確化

状態遷移にヒステリシスを追加

ブザーの異常時 noTone() を追加

タイマー周期を整理

---

### Q2: 単体テスト仕様の確認

> 「Section 5 の単体テスト仕様書で、各関数の動作が正しく検証できていますか？テストが不足している項目や、境界値テストが必要な箇所を教えてください。」

AIの回答（要約）：

距離測定の正常値・異常値・境界値テストが揃っている

状態遷移の境界値（50cm / 55cm）テストが必要

ブザー制御の state=0/1/異常値 のテストが揃っている

タイミング（100ms / 300ms）と並行動作の確認ができている

対応した内容：

境界値（48〜52cm）のテストを追加

ブザー異常時の noTone() テストを追加

タイミングテストを簡潔化

---

## 7. グループレビュー記録

### 7-1. 指摘一覧

| No | 指摘内容 | 指摘者 | 対応 |
|:---|:---|:---|:---|
| 1 |  異常値の選定と判定、あとマイナスの値を考慮について| 和田さん、リクさん |  センサーが測れない値（0cm・400cm超・マイナス・タイムアウト）は全部異常として扱い、前回の正常な距離（lastValidDistance）で動作を安定させます。|
| 2 |  |  |  |
| 3 |  |  |  |

### 7-2. レビューを受けて変更した点

-・異常値の基準を明確化した  
-ヒステリシスの意図を明確にした

---

*初版: YYYY-MM-DD / AIレビュー: YYYY-MM-DD / グループレビュー後更新: YYYY-MM-DD*
