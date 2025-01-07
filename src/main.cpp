/*
 * This code is referred to the follows under EPL-2.0 OR Apache-2.0 license
 * https://github.com/eclipse-zenoh/zenoh-pico/tree/main/examples/arduino
 */

#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <BluetoothSerial.h>
#include <zenoh-pico.h>
#include <vector>
#include <string>
#include <M5GFX.h>
#include "config.h" // config.h をインクルードしてwifiのssidとpasswordを設定

std::vector<std::string> messages = {"ちょっと来て", "ご飯の時間だよ", "おはよう！早く起きて！！"};

// WiFi-specific parameters
#define CLIENT_OR_PEER 0 // 0: Client mode; 1: Peer mode
#if CLIENT_OR_PEER == 0
#define MODE "client"
#define CONNECT "tcp/192.168.1.27:7447" // If empty, it will scout /* need to edit */
#elif CLIENT_OR_PEER == 1
#define MODE "peer"
#define CONNECT "udp/224.0.0.225:7447#iface=en0"
#else
#error "Unknown Zenoh operation mode. Check CLIENT_OR_PEER value."
#endif

#define KEYEXPR "key/expression"

z_owned_publisher_t pub;
static int idx = 0;
z_owned_session_t session;
bool is_zenoh_connected = false;

void print_message(String msg, bool play_sound = true);

void data_handler(const z_sample_t *sample, void *arg)
{
  z_owned_str_t keystr = z_keyexpr_to_string(sample->keyexpr);
  std::string val((const char *)sample->payload.start, sample->payload.len);

  // only print on Serial
  Serial.print("[sub.pico] ");

  String msg;
  msg.concat(val.c_str());
  print_message(msg);

  z_str_drop(z_str_move(&keystr));
}

void print_message_with_delay(const char* message, bool newline = true, int delay_ms = 0) {
  print_message(message, newline);
  if (delay_ms > 0) {
    delay(delay_ms);
  }
}

void connect_to_wifi(const char* ssid, const char* password) {
  print_message("Connecting to WiFi...", false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
  print_message("OK", false);
}

z_owned_session_t initialize_zenoh_session() {
  z_owned_config_t config = z_config_default();
  zp_config_insert(z_config_loan(&config), Z_CONFIG_MODE_KEY, z_string_make(MODE));
  if (strcmp(CONNECT, "") != 0) {
    zp_config_insert(z_config_loan(&config), Z_CONFIG_CONNECT_KEY, z_string_make(CONNECT));
  }

  print_message("Opening Zenoh Session...", false);
  z_owned_session_t s = z_open(z_config_move(&config));
  if (!z_session_check(&s)) {
    // Handle error
  }
  return s;
}

void declare_zenoh_entities() {
  // Declare Zenoh publisher
  String msg;
  msg.concat("Declaring publisher for ");
  msg.concat(KEYEXPR);
  msg.concat("...");
  print_message(msg, false);
  pub = z_declare_publisher(z_session_loan(&session), z_keyexpr(KEYEXPR), NULL);
  if (!z_publisher_check(&pub)) {
    print_message("Unable to declare publisher for key expression!", false);
    is_zenoh_connected = false;
    return;
  }
  print_message("OK", false);

  // Declare Zenoh subscriber
  msg = "";
  msg.concat("Declaring subscriber on ");
  msg.concat(KEYEXPR);
  msg.concat("...");
  print_message(msg, false);
  z_owned_closure_sample_t callback = z_closure_sample(data_handler, NULL, NULL);
  z_owned_subscriber_t sub =
      z_declare_subscriber(z_session_loan(&session), z_keyexpr(KEYEXPR), z_closure_sample_move(&callback), NULL);
  if (!z_subscriber_check(&sub)) {
    print_message("Unable to declare subscriber.", false);
    is_zenoh_connected = false;
    return;
  }
  print_message("OK", false);
}

void reconnect_zenoh_session() {
  print_message("Reconnecting Zenoh Session...", false);
  session = initialize_zenoh_session();
  if (z_session_check(&session)) {
    is_zenoh_connected = true;
    print_message("Zenoh Session reconnected", false);

    // Start the receive and the session lease loop for zenoh-pico
    zp_start_read_task(z_session_loan(&session), NULL);
    zp_start_lease_task(z_session_loan(&session), NULL);

    declare_zenoh_entities();
  } else {
    is_zenoh_connected = false;
    print_message("Failed to reconnect Zenoh Session", false);
  }
}

void setup() {
  // Initialize Serial port
  Serial.begin(115200);
  delay(1000);

  // Initialize M5 Display
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setFont(&fonts::lgfxJapanGothicP_24); // 日本語フォントを設定

  // Initialize Speaker
  M5.Speaker.begin();

  print_message_with_delay("Initialization OK", false, 8000);

  print_message("Zenoh setup start,,,", false);

  connect_to_wifi(WIFI_SSID, WIFI_PASS);

  reconnect_zenoh_session();
  print_message("Zenoh setup finished!", false);
}

void update_display_with_idx() {
  M5.Lcd.fillRect(0, M5.Lcd.height() - 30, M5.Lcd.width(), 30, BLACK); // 行をクリア
  M5.Lcd.setCursor(0, M5.Lcd.height() - 30); // 左下にカーソルを設定
  M5.Lcd.printf("message: %s", messages[idx].c_str());
}

void publish_message() {
  char buf[256];
  sprintf(buf, "%s", messages[idx].c_str());
  /* do not print on Display */
  Serial.print("[pub.pico] ");
  Serial.println(buf);

  z_publisher_put_options_t options = z_publisher_put_options_default();
  options.encoding = z_encoding(Z_ENCODING_PREFIX_TEXT_PLAIN, NULL);
  if (z_publisher_put(z_publisher_loan(&pub), (const uint8_t *)buf, strlen(buf), &options)) {
    print_message("Error while publishing data");
  }
}

void loop() {
  if (!is_zenoh_connected) {
    reconnect_zenoh_session();
  }

  if (M5.BtnA.wasPressed()) {
    idx = (idx + 1) % messages.size(); // idxをインクリメントし、範囲を維持
    update_display_with_idx();
  }
  if (M5.BtnB.wasPressed()) {
    idx = (idx - 1 + messages.size()) % messages.size(); // idxをデクリメントし、範囲を維持
    update_display_with_idx();
  }
  if (M5.BtnC.wasPressed()) {
    publish_message();
  }
  M5.update();
  delay(100);
}

void print_message(String msg, bool play_sound)
{
  Serial.println(msg);

  // Print latest message on M5 Display
  M5.Lcd.clear();
  M5.Lcd.startWrite();
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.print(msg);
  M5.Lcd.endWrite();

  // Play sound if required
  if (play_sound) {
    M5.Speaker.tone(1000, 200); // 1000Hzの音を200ms鳴らす
  }

  // Update idx display
  update_display_with_idx();
}