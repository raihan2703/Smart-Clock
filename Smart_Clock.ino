#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <SPI.h>
#include <DHT.h>

//Initialization of Internet COnnections
const char* ssid = "RAFIF";
const char* pass = "Rafif123";

//Initialization for time area
const long utcOffsetInSeconds = 25200; //GMT+7

//Initialization NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

//Menampilkan bulan dalam bentuk nama bulan (string)
String bulan[13]{"", "Januari", "Februari", "Maret", "April", "Mei", "Juni", "Juli", "Agustus", "September", "Oktober", "November", "Desember"};

//Initialization MAX7219
#define HARDWARE_TYPE MD_MAX72XX :: FC16_HW
#define DIN 13
#define CS 15
#define CLK 14
#define MAX_DEVICES 4
MD_Parola matrix = MD_Parola(HARDWARE_TYPE, DIN, CLK, CS, MAX_DEVICES);

//Variabel untuk mengubah tanggal menuju char karena di scroll
char arrTgl[18];

//Initialization duration
int counter = 0;

//Initialization state awal
String posisi = "jam";

//Initialization for DHT Sensor
#define DHTPIN 12
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//Variabel untuk mengubah float ke char
char arrTemp[7];

void setup(){
  Serial.begin(115200);
  matrix.begin();
  matrix.displayClear();
  WiFi.begin(ssid, pass);
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500);
  }
  timeClient.begin();
  dht.begin();
}

void loop(){
  if(posisi == "jam"){
    baca_jam();
  }
  else if(posisi == "tanggal"){
    baca_tanggal();
  }
  else if(posisi == "suhu"){
    baca_suhu();
  }
}

void baca_jam(){
  timeClient.update();
  String jam_lengkap = timeClient.getFormattedTime();
  //Parsing jam, menit, detik
  String jam, menit, detik;
  jam = String(timeClient.getHours());
  menit = String(timeClient.getMinutes());
  detik = String(timeClient.getSeconds());
  Serial.println(jam_lengkap);
  //Menampilkan ke display
  matrix.setTextAlignment(PA_CENTER);
  if((timeClient.getSeconds()) % 2 == 0){
    matrix.print(jam + ":" + menit);
  } else {
    matrix.print(jam + " " + menit);
  }
  //Untuk menentukan 20 detik ditampilkan
  counter++;
  if(counter == 20){
    posisi = "tanggal";
    counter = 0;
    matrix.displayClear();
  }
  delay(1000);
}

void baca_tanggal(){
  timeClient.update();
  //Konfigurasi untuk mendapatkan tanggal, bulan, tahun
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = localtime(&epochTime);
  //Parsing tanggal, bulan, tahun
  int tgl = ptm->tm_mday;
  String sTgl = "";
  if(tgl < 10){
    sTgl = "0" + String(tgl);
  } else {
    sTgl = String(tgl);
  }
  int bln = ptm->tm_mon + 1;
  String array_bulan = bulan[bln];
  int thn = ptm->tm_year + 1900;

  //Penggabungan dari pemarsingan diatas
  String tanggal_sekarang = sTgl + "/" + array_bulan + "/" + String(thn);
  Serial.println(tanggal_sekarang);

  //Pengkonversian dari string ke char array
  tanggal_sekarang.toCharArray(arrTgl, 18);
  //Tampilkan ke display
  if(matrix.displayAnimate()){
    matrix.displayText(arrTgl, PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    matrix.displayReset();
    counter++;
    if(counter > 1){
      posisi = "suhu";
      counter = 0;
      matrix.displayClear();
    }
  }
  delay(100);
}

void baca_suhu(){
  float t = dht.readTemperature();
  String temp = String(t) + "C";
  temp.toCharArray(arrTemp, 7);
  matrix.setTextAlignment(PA_CENTER);
  matrix.print(arrTemp);
  counter++;
  if(counter == 3){
    posisi = "jam";
    counter = 0;
    matrix.displayClear();
  }
  delay(1000);
}
