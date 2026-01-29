# Miniature Smart Greenhouse — Version 1.0 TODO

このファイルは、ミニチュアスマートグリーンハウスプロジェクトの **バージョン 1.0** を完了するために必要な具体的なタスクを追跡します。

---

## ドキュメント

- [x] TODO (1/19)
- [x] 回路図 (1/23)
- [x] README (1/22)
- [x] GitHub プロジェクトのセットアップ (1/27)

---

## ハードウェア

- [x] 水槽／植物の準備と調達 (1/18)
- [x] ピンを基板にハンダ付け (ESP32／温度＋湿度／その他) (1/19)
- [x] 水槽が適切に覆われるように LED ストリップをハンダ付け (1/19)
- [x] 12 V 電源 → ステップダウンレールのセットアップ (1/20)
- [x] システム全体の配線と電源投入テスト (1/26)

---

## ソフトウェア

### テスト

- [x] ESP-IDFセットアップ・初期テスト（点滅例・etc) (1/17)
- [x] DS18B20（土壌温度）の初期テスト(1/17)
- [x] 初期PWMテスト (ファン) (1/21)
- [x] 初期PWMテスト (ヒーター) (1/21)
- [x] 初期PWMテスト (LED) (1/21)
- [x] 初期CAP-SW-12 (土壌水分) テスト (1/25)
- [x] 初期SHT31 テスト (1/25)
- [x] MOSFETの準備とはんだ付け (1/25)

### 開発

#### アーキテクチャ

- [x] 全体的なソフトウェアアーキテクチャ / レイアウト (1/19)
- [x] ドライバとライブラリの検索 / 追加 (1/24)
- [x] Mainのリファクタリング (1/26)

#### 読み取り関数　（read）

- [x] `float read_soil_temp_c();` (1/24)
- [x] `float read_soil_moisture_percent();` (1/22)
- [x] `float read_air_temp_c();` (1/24)
- [x] `float read_air_humid_percent();` (1/24)
- [x] `rtc_time_t read_current_time();` (1/24)

#### 更新関数　（update）

- [x] `time_state_t update_time_state(rtc_time_t current_time);` (1/20)
- [x] `fan_state_t update_fan_state(float temp_c, float humid_percent, fan_state_t current);` (1/19)
- [x] `led_state_t update_led_state(time_state_t current_time, float temp_c);` (1/20)
- [x] `heater_state_t update_heater_state(float temp_c, heater_state_t current);` (1/20)
- [x] `signal_led_state_t update_soil_moisture_led_state(signal_led_state_t current, float soil_moisture_percent);` (1/20)

#### 実行関数　（run）

- [x] `void run_fan(fan_state_t current);` (1/21)
- [x] `void run_led(led_state_t current);` (1/21)
- [x] `void run_signal_led(signal_led_state_t current);` (1/21)
- [x] `void run_heater(heater_state_t current);` (1/21)

---

## 障害と依存関係

### 現状
- 無

### 解決済み

- [x] DS1307 (RTC) の注文が必要 — 1/20 → 1/20
- [x] SHT31 の初期テスト (モジュールは川滝の現場にある) — 1/19 → 1/23
- [x] MOSFET の準備とはんだ付け (部品は川滝の現場にある) — 1/19 → 1/23
- [x] 注文済みの DS1307 を待機中 (部品は川滝の現場にある) — 1/20 → 1/23

---

*EOF*