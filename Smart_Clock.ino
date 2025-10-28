#include <ArduinoOTA.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <SPI.h>
#include <DHT.h>
#include <WiFiManager.h>  // Library WiFi Manager

// ---------- Inisialisasi WiFi ----------
WiFiManager wifiManager;

// ---------- Inisialisasi NTP ----------
const long utcOffsetInSeconds = 25200; // GMT+7
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// ---------- Inisialisasi MAX7219 ----------
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define DIN 13
#define CS 15
#define CLK 14
#define MAX_DEVICES 4
MD_Parola matrix = MD_Parola(HARDWARE_TYPE, DIN, CLK, CS, MAX_DEVICES);

// ---------- Inisialisasi DHT11 ----------
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ---------- Variabel Global ----------
String bulan[13] = {
  "", "Januari", "Februari", "Maret", "April", "Mei", "Juni",
  "Juli", "Agustus", "September", "Oktober", "November", "Desember"
};

// --------- Bikin Array yang bisa ditampilkan
char arrTgl[30];
char arrTemp[10];

//--------- Pin LDR -------------------
#define ldrPin A0

#define touchPin 4
bool touchDetected = false;

int counter = 0;
String posisi = "jam";

void setup() {
  Serial.begin(115200);
  matrix.begin();
  matrix.displayClear();
  dht.begin();
  pinMode(touchPin, INPUT);

  // ---------- WiFi Manager ----------
  // Jika gagal konek, ESP akan jadi Access Point (contoh SSID: SmartClock_Setup)
  wifiManager.resetSettings();
  wifiManager.autoConnect("SmartClock_Setup");
  Serial.println("WiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // ---------- OTA Setup ----------
  ArduinoOTA.setHostname("SmartClock_ESP8266"); // nama perangkat saat OTA
  ArduinoOTA.onStart([]() {
    Serial.println("Mulai OTA update...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nSelesai OTA update!");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
  });
  ArduinoOTA.begin();
  Serial.println("OTA ready. Silakan upload sketch melalui jaringan WiFi yang sama.");

  timeClient.begin();
}

void loop() {
  int touched = digitalRead(touchPin);
  if (touched == HIGH) {
    touchDetected = true;
  }
  int ldrValue = analogRead(ldrPin);
  int brightness = map(ldrValue, 0, 1023, 0, 15);
  matrix.setIntensity(brightness);
  if (posisi == "jam") {
    baca_jam();
  } else if (posisi == "tanggal") {
    baca_tanggal();
  } else if (posisi == "suhu") {
    baca_suhu();
  }
}

void gantiMode() {
  if (posisi == "jam") posisi = "tanggal";
  else if (posisi == "tanggal") posisi = "suhu";
  else if (posisi == "suhu") posisi = "jam";

  matrix.displayClear();
  Serial.print("Mode berubah ke: ");
  Serial.println(posisi);
}


void baca_jam() {
  timeClient.update();
  int jamInt = timeClient.getHours();
  int menitInt = timeClient.getMinutes();

  // Format dua digit
  String jam = (jamInt < 10 ? "0" + String(jamInt) : String(jamInt));
  String menit = (menitInt < 10 ? "0" + String(menitInt) : String(menitInt));

  String jam_lengkap = jam + ":" + menit;
  Serial.println(jam_lengkap);

  // Tampilkan ke display
  matrix.setTextAlignment(PA_CENTER);
  if ((timeClient.getSeconds()) % 2 == 0) {
    matrix.print(jam + ":" + menit);
  } else {
    matrix.print(jam + " " + menit);
  }

  counter++;
  if (touchDetected || counter >= 20) {  // 20 detik tampil
    //posisi = "tanggal";
    touchDetected = false;
    counter = 0;
    gantiMode();
    matrix.displayClear();
  }
  delay(1000);
}

void baca_tanggal() {
  timeClient.update();
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = localtime(&epochTime);

  int tgl = ptm->tm_mday;
  int bln = ptm->tm_mon + 1;
  int thn = ptm->tm_year + 1900;

  String sTgl = (tgl < 10 ? "0" + String(tgl) : String(tgl));
  String tanggal_sekarang = sTgl + " " + bulan[bln] + " " + String(thn);

  tanggal_sekarang.toCharArray(arrTgl, sizeof(arrTgl));
  Serial.println(tanggal_sekarang);
  if(touchDetected){
    touchDetected = false;
    posisi = "suhu";
    //matrix.displaySuspend(true);
    counter = 0;
    matrix.displayReset();
    matrix.displayClear();
    delay(1000);
  } else {

  if (matrix.displayAnimate()) {
    
    matrix.displayText(arrTgl, PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    matrix.displayReset();
    counter++;
    if (counter >= 2) {
      posisi = "suhu";
      //touchDetected = false;
      counter = 0;
      //gantiMode();
      matrix.displayClear();
    }
  }
  }
  delay(100);
}

void baca_suhu() {
  float t = dht.readTemperature();
  //if (isnan(t)) {
   // Serial.println("Gagal membaca sensor DHT!");
    //return;
  //}

  String temp = String(t, 1) + "C";
  temp.toCharArray(arrTemp, sizeof(arrTemp));

  matrix.setTextAlignment(PA_CENTER);
  matrix.print(arrTemp);
  Serial.print("Suhu: ");
  Serial.println(temp);

  counter++;
  if (touchDetected || counter >= 5) {  // tampil selama 5 detik
    //posisi = "jam";
    touchDetected = false;
    counter = 0;
    gantiMode();
    matrix.displayClear();
  }
  delay(1000);
}
