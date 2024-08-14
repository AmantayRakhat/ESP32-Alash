#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <ESP_Mail_Client.h>

#define WIFI_SSID "REPLACE_WITH_YOUR_SSID"
#define WIFI_PASSWORD "REPLACE_WITH_YOUR_PASSWORD"

/** SMTP хост, например smtp.gmail.com для GMail или smtp.office365.com для Outlook или smtp.mail.yahoo.com */
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

/* Учетные данные для входа */
#define AUTHOR_EMAIL "YOUR_EMAIL@XXXX.com"
#define AUTHOR_PASSWORD "YOUR_EMAIL_APP_PASS"

/* Email получателя */
#define RECIPIENT_EMAIL "RECIPIENTE_EMAIL@XXXX.com"

/* Объявляем объект SMTPSession для SMTP-транспорта */
SMTPSession smtp;

/* Callback функция для получения статуса отправки Email */
void smtpCallback(SMTP_Status status);

void setup(){
  Serial.begin(115200);
  Serial.println();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Подключение к Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Подключен с IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Установить опцию повторного подключения к сети */
  MailClient.networkReconnect(true);

  /** Включить отладку через Serial порт
   * 0 для отключения отладки
   * 1 для базовой отладки
   */
  smtp.debug(1);

  /* Установить callback функцию для получения результатов отправки */
  smtp.callback(smtpCallback);

  /* Объявляем Session_Config для пользовательских учетных данных сессии */
  Session_Config config;

  /* Установить конфигурацию сессии */
  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;
  config.login.user_domain = "";

  /*
  Установить конфигурацию времени через NTP
  Для времени к востоку от Гринвича используйте 0-12
  Для времени к западу от Гринвича добавьте 12 к смещению.
  Например, для американского времени/Денвер GMT будет -6. 6 + 12 = 18
  Смотрите https://en.wikipedia.org/wiki/Time_zone для списка смещений часовых поясов GMT/UTC
  */
  config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  config.time.gmt_offset = 3;
  config.time.day_light_offset = 0;

  /* Объявляем класс сообщения */
  SMTP_Message message;

  /* Установить заголовки сообщения */
  message.sender.name = F("ESP");
  message.sender.email = AUTHOR_EMAIL;
  message.subject = F("Тестовое письмо от ESP");
  message.addRecipient(F("Sara"), RECIPIENT_EMAIL);
    
  /* Отправка сообщения в формате HTML */
  /*String htmlMsg = "<div style=\"color:#2f4468;\"><h1>Привет, мир!</h1><p>- Отправлено с платы ESP</p></div>";
  message.html.content = htmlMsg.c_str();
  message.html.charSet = "utf-8";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;*/

   
  // Отправка сообщения в формате обычного текста
  String textMsg = "Привет, мир! - Отправлено с платы ESP";
  message.text.content = textMsg.c_str();
  message.text.charSet = "utf-8";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
  
  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

  /* Подключение к серверу */
  if (!smtp.connect(&config)){
    ESP_MAIL_PRINTF("Ошибка подключения, Код состояния: %d, Код ошибки: %d, Причина: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return;
  }

  if (!smtp.isLoggedIn()){
    Serial.println("\nЕще не вошел в систему.");
  }
  else{
    if (smtp.isAuthenticated())
      Serial.println("\nУспешный вход.");
    else
      Serial.println("\nПодключен без авторизации.");
  }

  /* Начать отправку Email и закрыть сессию */
  if (!MailClient.sendMail(&smtp, &message))
    ESP_MAIL_PRINTF("Ошибка отправки Email, Код состояния: %d, Код ошибки: %d, Причина: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());

}

void loop(){
}

/* Callback функция для получения статуса отправки Email */
void smtpCallback(SMTP_Status status){
  /* Печать текущего статуса */
  Serial.println(status.info());

  /* Печать результата отправки */
  if (status.success()){
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Успешно отправлено сообщение: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Сообщение не отправлено: %d\n", status.failedCount());
    Serial.println("----------------\n");

    for (size_t i = 0; i < smtp.sendingResult.size(); i++){
      /* Получаем элемент результата */
      SMTP_Result result = smtp.sendingResult.getItem(i);

      ESP_MAIL_PRINTF("Сообщение №: %d\n", i + 1);
      ESP_MAIL_PRINTF("Статус: %s\n", result.completed ? "успех" : "неудача");
      ESP_MAIL_PRINTF("Дата/Время: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
      ESP_MAIL_PRINTF("Получатель: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Тема: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");

    // Необходимо очистить результаты отправки, так как использование памяти будет увеличиваться.
    smtp.sendingResult.clear();
  }
}
