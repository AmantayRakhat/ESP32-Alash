// Импорт необходимых библиотек
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

// Замените на ваши сетевые данные
const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

// Установите GPIO для светодиода
const int ledPin = 2;
// Сохраняет состояние светодиода
String ledState;

// Создайте объект AsyncWebServer на порту 80
AsyncWebServer server(80);

// Заменяет заполнитель значением состояния светодиода
String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    if(digitalRead(ledPin)){
      ledState = "ON";
    }
    else{
      ledState = "OFF";
    }
    Serial.print(ledState);
    return ledState;
  }
  return String();
}
 
void setup(){
  // Последовательный порт для целей отладки
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  // Инициализация SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("Произошла ошибка при монтировании SPIFFS");
    return;
  }

  // Подключение к Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Подключение к WiFi..");
  }

  // Печать локального IP-адреса ESP32
  Serial.println(WiFi.localIP());

  // Маршрут для корня / веб-страницы
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Маршрут для загрузки файла style.css
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Маршрут для установки GPIO в HIGH
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, HIGH);    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Маршрут для установки GPIO в LOW
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, LOW);    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Запуск сервера
  server.begin();
}
 
void loop(){
  
}
