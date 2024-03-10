// Подключаем библиотеку
#include "Wire.h"
#include "Adafruit_I2CDevice.h"
#include <Adafruit_Sensor.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <Arduino.h>

// Определяем константы для Firebase Database URL и API Key
#define DATABASE_URL "YOUR_DATABASE_URL"
#define API_KEY "YOUR_API_KEY"

// Определяем пины и переменные
const int ledPin = 2;
const int dhtPin = 18;
const char* wifiSsid = "YOUR_WIFI_SSID";
const char* wifiPass = "YOUR_WIFI_PASS";

unsigned long sendDataPrevMillis = 0;
const long sendDataIntervalMillis = 10000;
bool signupOK = false;
bool status;
const int maxRetryAttempts = 3;

Adafruit_BMP280 bmp;

DHT dht22(dhtPin, DHT22);

FirebaseData firebaseData;
FirebaseAuth firebaseAuth;
FirebaseConfig firebaseConfig;

// Функция для отправки данных с перезапросом
void sendDataWithRetry(const String& path, float value) {
    int retryAttempts = 0;
    bool success = false;

    while (!success && retryAttempts < maxRetryAttempts) {
        if(Firebase.RTDB.setFloat(&firebaseData, path, value)) {
            Serial.print("Успешная запись в ");
            Serial.println(path);
            success = true;
        } else {
            Serial.print("Не удалось записать в ");
            Serial.println(path);
            Serial.print("REASON: ");
            Serial.println(firebaseData.errorReason());

            retryAttempts++;
            delay(1000);
        }
    }

    if (!success) {
        Serial.println("Превышено количество попыток записи");
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(100);
    pinMode(ledPin, OUTPUT);

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSsid, wifiPass);
    Serial.print("Подключение к WIFI...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        digitalWrite(ledPin, HIGH);
        delay(500);
        digitalWrite(ledPin, LOW);

        delay(500);
        Serial.print('.');
    }
    Serial.println();
    Serial.print("Успешное подключение к WIFI... ");
    Serial.print(WiFi.localIP());
    Serial.println();

    firebaseConfig.api_key = API_KEY;
    firebaseConfig.database_url = DATABASE_URL;

    Serial.println("Регистрация нового пользователя...");

    if (Firebase.signUp(&firebaseConfig, &firebaseAuth, "", "")) {
        Serial.println("Регистрация прошла успешно...");
        signupOK = true;
    } else {
        Serial.printf("%s\n", firebaseConfig.signer.signupError.message.c_str());
    }

    Serial.println("");

    status = bmp.begin(0x76);
    dht22.begin();

    // Устанавливаем обратный вызов для обработки состояния токена
    firebaseConfig.token_status_callback = tokenStatusCallback;

    Firebase.begin(&firebaseConfig, &firebaseAuth);
    Firebase.reconnectWiFi(true);
}

void loop() {
    
    // Проверяем состояние датчика BMP280 и выводим сообщение в случае отсутствия связи
    if (!status) {
        Serial.println(F("Не удалось найти действительный датчик BMP280, проверьте подключение или "
                        "попробуйте другой адрес!"));
        while (!status) {
            delay(100);
            digitalWrite(ledPin, HIGH);
            delay(100);
            digitalWrite(ledPin, LOW);
        }
    }

    // Считываем данные с датчиков
    float BMPtemp = bmp.readTemperature();
    float DHTtemp = dht22.readTemperature();
    float pressure = bmp.readPressure();
    int millimeterOfMercury;
    millimeterOfMercury = pressure * 0.00750062;
    float humidity = dht22.readHumidity();

    // Обработка ситуации, если данные с датчика DHT22 некорректны
    if (isnan(DHTtemp) || isnan(humidity)) {
        Serial.println(F("Не удалось считать данные DHT22"));
        while (isnan(DHTtemp) || isnan(humidity)) {
            delay(100);
            digitalWrite(ledPin, HIGH);
            delay(100);
            digitalWrite(ledPin, LOW);
            DHTtemp = dht22.readTemperature();
            humidity = dht22.readHumidity();
        }
    }

    // Обработка ситуации, если данные с датчика BMP280 некорректны
    if (BMPtemp > 100 || millimeterOfMercury < 0) {
        Serial.println(F("Не удалось считать данные BMP280"));
        while (BMPtemp > 100 || millimeterOfMercury < 0) {
            delay(100);
            digitalWrite(ledPin, HIGH);
            delay(100);
            digitalWrite(ledPin, LOW);
            BMPtemp = bmp.readTemperature();
            pressure = bmp.readPressure();
            millimeterOfMercury = pressure * 0.00750062;
        }
    }

    // Выводим данные в Serial Monitor
    Serial.print(F("Температура BMP280 = "));
    Serial.print(BMPtemp);
    Serial.println(" *C");

    Serial.print(F("Температура DHT22 = "));
    Serial.print(DHTtemp);
    Serial.println(" *C");

    Serial.print(F("Давление = "));
    Serial.print(millimeterOfMercury);
    Serial.println(" мм рт.ст.");

    Serial.print(F("Влажность = "));
    Serial.print(humidity);
    Serial.println(" %");

    Serial.println();
    delay(1000);

    // Отправляем данные в Firebase Realtime Database с перезапросом
    if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > sendDataIntervalMillis || sendDataPrevMillis == 0)) {
        sendDataPrevMillis = millis();

        sendDataWithRetry("/BMP280/temperature", BMPtemp);
        sendDataWithRetry("/BMP280/millimeterOfMercury", millimeterOfMercury);
        sendDataWithRetry("/DHT22/humidity", humidity);
        sendDataWithRetry("/DHT22/temperature", DHTtemp);
    }
}