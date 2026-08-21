#include <M5Unified.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <SpotifyEsp32.h>
#include <HTTPClient.h> // to print album covers to screen (json uses http)
#include <WiFiClientSecure.h> // including to try and fix HTTP error code: -7
#include <secrets.h> // includes wifi/passwords + spotify tokens

// spotify instance
Spotify sp(CLIENT_ID, CLIENT_SECRET, REFRESH_TOKEN);

// used to print images
HTTPClient http; 
WiFiClientSecure secureClient;

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
  secureClient.setInsecure();
  secureClient.setTimeout(5000);
}


void loop() {
  M5.update();

  static unsigned long last_check = 0;

  // Correct array initialization syntax
  static char last_song[64] = "";
  static char last_artist[64] = "";
  static char last_album_url[128] = ""; // not sure if buffer too long

  if (millis() - last_check > 3000) {
    last_check = millis(); 
    JsonDocument playback;
    response res = sp.get_currently_playing_track(playback);
    const char* song_name = res.reply["item"]["name"];
    const char* artist_name = res.reply["item"]["artists"][0]["name"];
    // 300x300 size
    const char* album_url = res.reply["item"]["album"]["images"][1]["url"];


    //  protection against null pointers
    if (song_name == nullptr) {
      song_name = "No Active Track";
    }
    if (artist_name == nullptr) {
      artist_name = "Unknown Artist";
    }

    if (strcmp(last_song, song_name) != 0 || strcmp(last_artist, artist_name) != 0 || strcmp(last_album_url, album_url) != 0) {
      
      // Print the JSON so we can diagnose connection issues
      // debug: serializeJsonPretty(res.reply, Serial);
      Serial.println(); 

      M5.Lcd.fillScreen(TFT_BLACK);
      M5.Lcd.setCursor(0, 0);

      M5.Lcd.printf("Song: %s\n",song_name);
      M5.Lcd.printf("Artist: %s\n", artist_name);

      strlcpy(last_song, song_name, sizeof(last_song));
      strlcpy(last_artist, artist_name, sizeof(last_artist));
      strlcpy(last_album_url, album_url, sizeof(last_album_url));
    }

    // logic for printing album
    if (album_url != nullptr) {
      print_album_cover(album_url);
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

void print_album_cover(const char* image_url) {
  int http_code = -1;
  if(http.begin(secureClient, image_url)) {
    http_code = http.GET();
  }

  if (http_code == HTTP_CODE_OK) {
    int total_bytes = http.getSize();
    if (total_bytes <= 0) {
      M5.Lcd.println("Album image has bad content length");
      http.end();
      return;
    }
    WiFiClient* stream = http.getStreamPtr();

    // don't have ssd, using ram to allocate incoming data
    uint8_t* buffer = (uint8_t*)malloc(total_bytes);

    if (buffer) {
      // keeping track of read state so if only partially reads bytes wont stop
      int bytes_read = 0;
      unsigned long data_time = millis();

      // adding 5s timeout/longer read: had error with breaking too early and not reading all the data
      // would never actaully print album cover if image was larger than 64x64
      while (bytes_read < total_bytes && (millis() - data_time) < 5000) {
        if (stream -> available()) {
          int c = stream -> readBytes(buffer + bytes_read, total_bytes - bytes_read);
          if (c > 0) {
            bytes_read += c;
            data_time = millis();
          }
          else if (!http.connected()) {
            break;
          }
        }
      }

      if (bytes_read == total_bytes) {
        M5.Lcd.drawJpg(buffer, total_bytes, 55, 40, 180, 180);
      } 
      
      else{
        M5.Lcd.printf("Incomplete read: %d/%d\n", bytes_read, total_bytes);
      }
    } 
    
    else {
      M5.Lcd.println("Error: Not enough ram for buffer");
    }

    free(buffer);
  } 
  
  else {
    M5.Lcd.printf("HTTP Error code: %d", http_code);
  }

  http.end();
  secureClient.stop(); 
}