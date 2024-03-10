# Мини-метеостанция на основе контроллера ESP32, а также датчиках BMP280 и DHT22

Использованные включенные библиотеки:
  	adafruit/Adafruit BMP280 Library@^2.6.8,
	adafruit/Adafruit Unified Sensor@^1.1.14,
	adafruit/DHT sensor library@^1.4.6,
	mobizt/Firebase Arduino Client Library for ESP8266 and ESP32@^4.4.12

Схема подключения датчика DHT22:
	+ или VCC ----> 3.3V пин;
 	- или GND ----> Любой GND пин;
  	out или data ----> GPIO18;
   
Схема подключения датчика BMP280:
	+ или VCC ----> 3.3V пин;
 	- или GND ----> Любой GND пин;
  	SCL ----> GPIO22;
   	SDA ----> GPIO21;

Отправка данных происходит в Realtime database Firebase, поэтому потребуется регистрация на сервисе.
