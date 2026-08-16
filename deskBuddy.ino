#include <M5Unified.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <SpotifyEsp32.h>
#include <secrets.h> // includes wifi/passwords + spotify tokens

// spotify instance
Spotify sp(client_id, client_secret, user_refresh_token);

void setup() {
  // turns on screen/power management chip
  M5.begin();
  Serial.begin(115200);

  M5.Lcd.setTextSize(2);
  M5.Lcd.println("Connecting to Wifi...");

  // connect to the wifi
  WiFi.begin(ssid, password);
  // loop until connect, make make a timeout if password set incorrect
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    M5.Lcd.println(".");
  }

  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setCursor(0,0);
  M5.Lcd.println("Wifi conencted!");
  delay(500);

  // setting up spotify perms
  sp.set_scopes("user-read-currently-playing user-read-playback-state user-modify-playback-state");
  sp.begin();

  M5.Lcd.println("Linking to spotify...");
  // wait for the auth for spotify
  while(!sp.is_auth()) {
    sp.handle_client();
  }

  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setCursor(0,0);
  M5.Lcd.println("Spotify Linked!");

  /* had issues grabbing refresh so I didn't have to auth each time .
  Serial.print("Your refresh token is: ");
  Serial.println(sp.get_user_tokens().refresh_token);
  copy and pasted what was printed into the spotify instance so would auto link*/ 

}

void loop() {
  M5.update();
  M5.Lcd.println("get back to work");
  exit(0);

}