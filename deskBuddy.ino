#include <M5Unified.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <SpotifyEsp32.h>
#include <secrets.h> // includes wifi/passwords + spotify tokens
#include <string>

// spotify instance
Spotify sp(CLIENT_ID, CLIENT_SECRET, REFRESH_TOKEN);

#define BAURDATE 115200

void setup() {
  // turns on screen/power management chip
  M5.begin();
  Serial.begin(BAURDATE);

  M5.Lcd.setTextSize(2); 
  wifi_connect();


  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setCursor(0,0);
  delay(500);

  // setting up spotify perms
  sp.set_scopes("user-read-currently-playing user-read-playback-state user-modify-playback-state");
  sp.begin();

  // wait for the auth for spotify
  while(!sp.is_auth()) {
    sp.handle_client();
  }

  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setCursor(0,0);
}

// TODO: fix program restarting, and make it so artist name prints, then figure out album cover

void loop() {
  M5.update();


  // need to use static since in a loop function
  static unsigned long last_check = 0;

  // using fixed size char arrays instead of const char* to hold text
  static char last_song[64] = "";
  static char last_artist[64] = "";

  // 3 seconds have passed
  if (millis() - last_check > 3000) {
    last_check = millis(); 
    JsonDocument playback;
    response res = sp.get_currently_playing_track(playback);

    const char* song_name = res.reply["item"]["name"];
    // "artists" is an array in the json
    const char* artist_name = res.reply["item"]["artists"][0]["name"];

    // insert logic for null ptrs if no data

    if (strcmp(last_song, song_name) != 0 || strcmp(last_artist, artist_name) != 0) {
      M5.Lcd.fillScreen(TFT_BLACK);
      M5.Lcd.setCursor(0,0);

      // prints text only when changes, so no blinking
      M5.Lcd.println(song_name);
      M5.Lcd.println(artist_name);

      // strlcpy auto stops at song length and forces null termination (easier than strncpy in this case)
      strlcpy(last_song, song_name, sizeof(last_song));
      strlcpy(last_artist, artist_name, sizeof(last_artist));
    }
  }
}

void wifi_connect() {
  WiFi.begin(SSID, PASSWORD);
  M5.Lcd.print("Connecting to WiFi");
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
    M5.Lcd.print(".");
  }
  M5.Lcd.println("...Connected to WiFi");
}