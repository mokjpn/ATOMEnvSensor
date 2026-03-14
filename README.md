# ATOMEnvSensor
Measure environment with M5Stack EnvIII Unit 

## 概要
M5Stack ATOM S3 Lite に M5Stack ENV3 Unit を接続して、環境データ（温度、湿度、気圧）を10秒ごとに測定し、MQTTブローカーにpublishするArduinoスケッチです。

## 必要なハードウェア
- M5Stack ATOM S3 Lite
- M5Stack Atomic ToUnit Base
- M5Stack ENV3 Unit

## セットアップ

### 1. 設定ファイルの作成
```bash
cp include/secrets-example.h include/secrets.h
```

`include/secrets.h` を編集して、WiFiとMQTTブローカーの情報を設定してください：
- `WIFI_SSID`: WiFiのSSID
- `WIFI_PASSWORD`: WiFiのパスワード
- `MQTT_BROKER`: MQTTブローカーのホスト名またはIPアドレス
- `MQTT_PORT`: MQTTブローカーのポート（通常は1883）
- `MQTT_CLIENT_ID`: MQTTクライアントID
- `MQTT_USER`: MQTTユーザー名
- `MQTT_PASSWORD`: MQTTパスワード
- `MQTT_TOPIC`: データをpublishするMQTTトピック

### 2. ビルドとアップロード
PlatformIOを使用してビルドとアップロードを行います：

```bash
# ビルド
pio run

# ビルドとアップロード
pio run -t upload

# シリアルモニタ
pio device monitor
```

## データフォーマット
センサーデータはJSON形式でMQTTトピックにpublishされます：

```json
{
  "temperature": 25.30,
  "humidity": 45.50,
  "pressure": 1013.25
}
```

- `temperature`: 温度（℃）
- `humidity`: 湿度（%）
- `pressure`: 気圧（hPa）

## 接続
このスケッチは `src/main.cpp` で `Wire.begin(8, 5)` を使用しているため、ENV3 Unit は ATOM S3 Lite 本体の HY2.0-4P ポートではなく、M5Stack Atomic ToUnit Base 経由で接続してください。

Atomic ToUnit Base の HY2.0-4P コネクターに ENV3 Unit を接続し、DIP スイッチで I2C 信号が次のピンに割り当てられるように設定します。
- SDA: GPIO8（HY2.0-4P の Yellow / IO1）
- SCL: GPIO5（HY2.0-4P の White / IO2）

ATOM S3 Lite 本体の HY2.0-4P ポートを使う場合の I2C ピンは GPIO2 / GPIO1 ですが、現在のソースコードはその設定では動作しません。

ATOM S3 Lite 本体の HY2.0-4P ポートを使いたい場合は、`src/main.cpp` の I2C 初期化を次のように変更してください。

```cpp
// Wire.begin(2, 1); // SDA=GPIO2, SCL=GPIO1 for ATOM S3
Wire.begin(2, 1);
```

その場合は、ToUnit Base 側の DIP スイッチ設定は不要です。
