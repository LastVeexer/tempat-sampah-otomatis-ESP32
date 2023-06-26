#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <FirebaseESP32.h>

//Wifi Setting
const char* ssid = "realme 9";
const char* password = "nanyaapabro?";

// const char* ssid = "Andromeda";
// const char* password = "Fiesta47";

//bot setting
#define BOTtoken "5875653809:AAFF0cuPjZzVrjLLFN5i0qywH5BZYDZv6iE"
// #define CHAT_ID "1484540644" ARTHA
#define CHAT_ID "-884759265" //CHANNEL BOT

// Informasi Firebase
const char * FIREBASE_HOST = "tempatsampahcerdas-ffe89-default-rtdb.firebaseio.com/"; // Database URL
const char * FIREBASE_AUTH = "wB1p72ecKZzRCdY9ElhFzMXqc5LfXqfMb0gGTOnU"; // Database Secrets

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

#define FIREBASE_OK 200

FirebaseESP32 firebase;
FirebaseData firebaseData;
FirebaseJson json;

// Pesan yang diterima
String location;


int botRequestDelay = 1000;
unsigned long lastTimeBotRan;


// Servo
Servo myservo;

// LCD
LiquidCrystal_I2C lcd(0x27, 20, 4);
String lcd_text_line_1, lcd_text_line_2;

long timer_outdoor, timer_indoor;
float jarak_outdoor, jarak_indoor;

// Setting Ultrasonic
const int ultrasonic_indoor_trigger = 5;
const int ultrasonic_indoor_echo = 18;
const int ultrasonic_outdoor_trigger = 19;
const int ultrasonic_outdoor_echo = 21;

void handleNewMessages(int numNewMessages){
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));
  for (int i=0; i<numNewMessages; i++){
    String chat_id = String(bot.messages[i].chat_id);

    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }

    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String welcome = "Selamat datang di Bot Sampah! Silakan gunakan perintah berikut:\n\n";
      welcome += "- /tambah_lokasi untuk menambahkan lokasi tempat sampah\n";
      welcome += "- /buka_tempat_sampah untuk membuka tempat sampah\n";
      welcome += "- /tutup_tempat_sampah untuk menutup tempat sampah\n";
      bot.sendMessage(chat_id, welcome, "");
    }

     if (text == "/tambah_lokasi") {
      // Prompt the user to enter the location
      bot.sendMessage(chat_id, "Masukkan lokasi tempat sampah:", "");

      // Wait for the location message
      while (true) {
        bot.getUpdates(bot.last_message_received + 1);
        if (bot.messages[i].text != "") {
          String location = bot.messages[i].text;
          // ...
          break;
        }
      }

      // Store the location in Firebase
      String path = String("/tempat_sampah/") + chat_id;
      Firebase.setString(firebaseData, path.c_str(), location.c_str());

      // Send a confirmation message
      bot.sendMessage(chat_id, "Lokasi tempat sampah telah ditambahkan.", "");
    }

    if (text == "/cek_lokasi") {
      // Retrieve the location from Firebase
      String path = String("/tempat_sampah/") + chat_id;
      if (Firebase.getString(firebaseData, path.c_str())) {
        int httpCode = firebaseData.httpCode();
        if (httpCode == FIREBASE_OK) {
          String savedLocation = firebaseData.stringData();
          if (savedLocation != "") {
            bot.sendMessage(chat_id, "Lokasi tempat sampah: " + savedLocation, "");
          } else {
            bot.sendMessage(chat_id, "Lokasi tempat sampah belum ditambahkan.", "");
          }
        } else {
          Serial.println("Failed to retrieve location from Firebase");
        }
      }
    }

    if (text == "/buka_tempat_sampah"){
      myservo.write(180);
      bot.sendMessage(chat_id, "Tempat sampah telah dibuka.", "");
      // digitalWrite(ultrasonic_indoor_trigger, LOW);
      // digitalWrite(ultrasonic_outdoor_trigger, LOW);

      while (true) {
        bot.getUpdates(bot.last_message_received + 1);
        if (bot.messages[i].text != "/tutup_tempat_sampah") {
            // Menutup tempat sampah
            myservo.write(0);
            // // Sensor indoor dan outdoor high
            // digitalWrite(ultrasonic_indoor_trigger, HIGH);
            // digitalWrite(ultrasonic_outdoor_trigger, HIGH);
          break;
        }
        else {
           bot.sendMessage(chat_id, "Silahkan Tutup Tempat Sampah Terlebih Dahulu!");
        }
      }
    }

    if (text == "/tutup_tempat_sampah"){
      myservo.write(0);
      bot.sendMessage(chat_id, "Tempat sampah telah ditutup.", "");
    }

    if (text == "/cek_sampah"){
      digitalWrite (ultrasonic_indoor_trigger, LOW);
      delayMicroseconds (2);
      digitalWrite (ultrasonic_indoor_trigger, HIGH);
      delayMicroseconds (10);
      digitalWrite (ultrasonic_indoor_trigger, LOW);

      long timer_indoor = pulseIn(ultrasonic_indoor_echo, HIGH);
      float jarak_indoor = timer_indoor / 58.2;

      if (jarak_indoor <= 5) {
        lcd_text_line_1 = "    SAMPAH PENUH!    ";
        lcd_text_line_2 = " Jangan Diisi Lagi! ";
        lcd.setCursor(0, 0);
        lcd.print(lcd_text_line_1);
        lcd.setCursor(0, 1);
        lcd.print(lcd_text_line_2);

        digitalWrite(ultrasonic_outdoor_trigger, LOW);

        myservo.write(0);

        bot.sendMessage(chat_id, "Sampah Penuh! Segera Kosongkan Tempat Sampah.", "");
      }
      else{
        bot.sendMessage(chat_id, "Tempat sampah masih dapat diisi.", "");
      }
    }
  }
}
void setup() {
  Serial.begin(115200);
  // Ultrasonic
  pinMode(ultrasonic_outdoor_trigger, OUTPUT);
  pinMode(ultrasonic_outdoor_echo, INPUT);
  pinMode(ultrasonic_indoor_trigger, OUTPUT);
  pinMode(ultrasonic_indoor_echo, INPUT);

  // LCD
  Wire.begin(27, 26);
  lcd.init();
  lcd.backlight();
  lcd.clear();

  // Servo
  myservo.attach(14);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());
}

void loop() {
  digitalWrite (ultrasonic_indoor_trigger, LOW);
  delayMicroseconds (2);
  digitalWrite (ultrasonic_indoor_trigger, HIGH);
  delayMicroseconds (10);
  digitalWrite (ultrasonic_indoor_trigger, LOW);

  long timer_indoor = pulseIn(ultrasonic_indoor_echo, HIGH);
  float jarak_indoor = timer_indoor / 58.2;

  if (jarak_indoor <= 5) {
    lcd_text_line_1 = "    SAMPAH PENUH!    ";
    lcd_text_line_2 = " Jangan Diisi Lagi! ";
    lcd.setCursor(0, 0);
    lcd.print(lcd_text_line_1);
    lcd.setCursor(0, 1);
    lcd.print(lcd_text_line_2);

    digitalWrite(ultrasonic_outdoor_trigger, LOW);

    myservo.write(0);

    bot.sendMessage(CHAT_ID, "Sampah Penuh! Segera Kosongkan Tempat Sampah.", "");
  }
  else {
    lcd_text_line_1 = "   Silahkan Buang   ";
    lcd_text_line_2 = "   Sampah  Disini   ";
    lcd.setCursor(0, 0);
    lcd.print(lcd_text_line_1);
    lcd.setCursor(0, 1);
    lcd.print(lcd_text_line_2);

    digitalWrite(ultrasonic_outdoor_trigger, LOW);
    delayMicroseconds(2);
    digitalWrite(ultrasonic_outdoor_trigger, HIGH);
    delayMicroseconds(10);
    digitalWrite(ultrasonic_outdoor_trigger, LOW);

    timer_outdoor = pulseIn(ultrasonic_outdoor_echo, HIGH);
    jarak_outdoor = timer_outdoor / 58.2;

    //kalau jaraknya kurang dari atau sama dengan 20 maka servo berputar 180 derajat
    if (jarak_outdoor <= 20) {
      myservo.write(180);
      digitalWrite(ultrasonic_indoor_trigger, LOW);
      delay(500);
    }

    //kalau jarak lebih dari 20 cm, servo tidak berputar
    else {
      myservo.write(0);
      delay(500);
    }
  }
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    // When a new message arrives, call the handleNewMessages function.
    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
  // Serial.print("jarak outdoor= ");
  // Serial.print(jarak_outdoor);
  // Serial.print(" cm");
  // Serial.println();
  // Serial.print("jarak indoor= ");
  // Serial.print(jarak_indoor);
  // Serial.print(" cm");
  // Serial.println();

}
