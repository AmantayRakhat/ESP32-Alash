#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <ESP_Mail_Client.h>

#define WIFI_SSID "REPLACE_WITH_YOUR_SSID"
#define WIFI_PASSWORD "REPLACE_WITH_YOUR_PASSWORD"

#define SMTP_HOST "smtp.gmail.com"

/** SMTP порт, например 
 * 25  или esp_mail_smtp_port_25
 * 465 или esp_mail_smtp_port_465
 * 587 или esp_mail_smtp_port_587
*/
#define SMTP_PORT 465

/* Учетные данные для входа */
#define AUTHOR_EMAIL "YOUR_EMAIL@XXXX.com"
#define AUTHOR_PASSWORD "YOUR_EMAIL_APP_PASS"

/* Email получателя*/
#define RECIPIENT_EMAIL "RECIPIENTE_EMAIL@XXXX.com"

/* Объект сессии SMTP, используемый для отправки Email */
SMTPSession smtp;

/* Callback функция для получения статуса отправки Email */
void smtpCallback(SMTP_Status status);

void setup(){
  Serial.begin(115200);
  Serial.println();
  Serial.print("Подключение к AP");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(200);
  }
  Serial.println("");
  Serial.println("Wi-Fi подключен.");
  Serial.println("IP адрес: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Инициализация файловой системы
  ESP_MAIL_DEFAULT_FLASH_FS.begin();

  /** Включить отладку через Serial порт
   * нет отладки или 0
   * базовая отладка или 1
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
  config.login.user_domain = "mydomain.net";
  
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

  /* Включить передачу данных с сегментацией для больших сообщений, если сервер поддерживает */
  message.enable.chunking = true;

  /* Установить заголовки сообщения */
  message.sender.name = "ESP Mail";
  message.sender.email = AUTHOR_EMAIL;

  message.subject = F("Тест отправки Email с вложениями и встроенными изображениями из Flash");
  message.addRecipient(F("Sara"), RECIPIENT_EMAIL);

  /** В этом примере отправляются две альтернативные версии контента, например, обычный текст и HTML */
  String htmlMsg = "Это сообщение содержит вложения: изображение и текстовый файл.";
  message.html.content = htmlMsg.c_str();
  message.html.charSet = "utf-8";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_qp;

  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_normal;
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

  /* Элемент данных вложения */
  SMTP_Attachment att;

  /** Установить информацию о вложении, например 
   * имя файла, MIME тип, путь к файлу, тип хранения файла,
   * кодировка передачи и кодировка содержимого
  */
  att.descr.filename = "image.png";
  att.descr.mime = "image/png"; 
  att.file.path = "/image.png";
  att.file.storage_type = esp_mail_file_storage_type_flash;
  att.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;

  /* Добавить вложение к сообщению */
  message.addAttachment(att);

  message.resetAttachItem(att);
  att.descr.filename = "text_file.txt";
  att.descr.mime = "text/plain";
  att.file.path = "/text_file.txt";
  att.file.storage_type = esp_mail_file_storage_type_flash;
  att.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;

  /* Добавить вложение к сообщению */
  message.addAttachment(att);

  /* Подключение к серверу с конфигурацией сессии */
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
  if (!MailClient.sendMail(&smtp, &message, true))
    Serial.println("Ошибка отправки Email, " + smtp.errorReason());
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

    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++){
      /* Получаем элемент результата */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Сообщение №: %d\n", i + 1);
      ESP_MAIL_PRINTF("Статус: %s\n", result.completed ? "успех" : "неудача");
      ESP_MAIL_PRINTF("Дата/Время: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Получатель: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Тема: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");

    // Необходимо очистить результаты отправки, так как использование памяти будет увеличиваться.
    smtp.sendingResult.clear();
  }
}
