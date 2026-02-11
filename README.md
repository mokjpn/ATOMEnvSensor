# ATOMEnvSensor
Measure environment with M5Stack EnvIII Unit 

## 概要
M5Stack ATOM S3 Lite に M5Stack ENV3 Unit を接続して、環境データ（温度、湿度、気圧）を10秒ごとに測定し、MQTTブローカーにpublishするArduinoスケッチです。

## 必要なハードウェア
- M5Stack ATOM S3 Lite
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
ENV3 UnitはATOM S3 LiteのGroveポート（I2C）に接続してください。
- SDA: GPIO2
- SCL: GPIO1
