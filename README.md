# ElephantPlantWatering

Описание
elephant_plant_watering_system_esp — это прошивка для главного контроллера (шлюза) распределенной системы умного мониторинга и полива растений. <br>
Данное устройство является "мозгом" системы: оно предоставляет Web-интерфейс для мониторинга состояния всех устройств и ручного/автоматического управления поливом. <br>
Шлюз собирает телеметрию с периферийных исполнительных узлов (на базе Arduino Nano) по радиоканалу (HC-12 или LoRa) и агрегирует её для отображения в браузере.<br>
Помимо сетевых функций, контроллер оснащен собственными локальными сенсорами для измерения атмосферного давления и температуры (BMP085/BMP180), а также модулем реального времени (RTC), который обеспечивает точное планирование задач полива и корректное логирование событий даже при сбоях сети.<br>
<br>Ключевые функции шлюза:<br>

    Хостинг Web-UI для управления всей системой полива.<br>
    Прием и обработка данных с периферийных узлов (Arduino Nano) по радиоканалу.<br>
    Измерение атмосферного давления и температуры окружающей среды (BMP085).<br>
    Отслеживание точного времени с помощью RTC-модуля для планирования расписания полива.<br>
    Ретрансляция команд управления на исполнительные устройства (включение/выключение насосов).<br><br>

Технологии и оборудование<br>
В данном проекте были использованы следующие технологии, библиотеки и компоненты:<br>

    C++ / Arduino Framework: Основной язык и фреймворк для написания прошивки.<br>
    PlatformIO: Среда и система сборки для управления зависимостями, компиляции и прошивки микроконтроллера.<br>
    Adafruit BMP085 Library: Библиотека для работы с барометром и датчиком температуры BMP085/BMP180.<br>
    RTClib: Библиотека для взаимодействия с часами реального времени (DS1307, DS3231 и совместимыми).<br>
    Микроконтроллер: ESP32 (в данном случае uPesy ESP32 Wroom DevKit).<br>
    Радиомодули: HC-12 или RAK811 (LoRa) для организации связи с периферийными узлами.<br><br>

Сборка и запуск<br>
Для успешной сборки и прошивки устройства Вам понадобится:<br>

    Visual Studio Code с установленным расширением PlatformIO IDE.<br>
    Все зависимости уже указаны в файле platformio.ini и подтянутся автоматически.<br>
    Подключенная плата ESP32 (в конфиге указан порт COM9, при необходимости измените upload_port и monitor_port в platformio.ini на ваш актуальный порт).<br>


Примечание: После прошивки ESP32 запустит локальный Web-сервер. IP-адрес и параметры подключения к вашей Wi-Fi сети (если используется) необходимо настроить в коде перед первой компиляцией.

    

## Лицензия

##### CC BY-NC 4.0 в следующей нотации:
  ###### RU
     Creative Commons Attribution-NonCommercial 4.0 Международная общедоступная лицензия
     
     Осуществляя Лицензионные права (определенные ниже), Вы принимаете и соглашаетесь соблюдать положения и условия настоящей публичной лицензии Creative Commons Attribution-NonCommercial 4.0 International ("Публичная лицензия"). В той мере, в какой эта Публичная лицензия может быть истолкована как договор, Вам предоставляются Лицензионные права при условии, что Вы принимаете настоящие положения и условия, а Лицензиар предоставляет Вам такие права с учетом выгод, которые Лицензиар получает от предоставления Лицензируемых материалов. на этих условиях.
    
     Вы можете:
     - Распространять — копируйте и распространяйте материал на любом носителе и в любом формате
     - Адаптировать — изменять, адаптировать и создавать на основе 
     
     На следующих условиях:
     - Авторство — вы должны предоставить ссылку на лицензию и указать, ссылку на репозиторий проекта, были ли внесены изменения. Вы можете сделать это любым разумным способом, но никоим образом не предполагающим, что лицензиар одобряет вас или ваше использование.
     - NonCommercial — Вы не можете использовать материал в коммерческих целях.
     
  ###### EN
    Creative Commons Attribution-NonCommercial 4.0 International Public License
    
    By exercising the Licensed Rights (defined below), You accept and agree to be bound by the terms and conditions of this Creative Commons Attribution-NonCommercial 4.0 International Public License ("Public License"). To the extent this Public License may be interpreted as a contract, You are granted the Licensed Rights in consideration of Your acceptance of these terms and conditions, and the Licensor grants You such rights in consideration of benefits the Licensor receives from making the Licensed Material available under these terms and conditions.
    
    You are free to:
    - Share — copy and redistribute the material in any medium or format
    - Adapt — remix, transform, and build upon the material
    
    Under the following terms:
    - Attribution — You must give appropriate credit, provide a link to the license,link to the github page project and indicate if changes were made. You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.
    - NonCommercial — You may not use the material for commercial purposes.

## Ответственность
###### RU
    Программный продукт, представленный в этом репозитории, предоставляется "как есть" без каких-либо явных или подразумеваемых гарантий, включая, но не ограничиваясь, подразумеваемыми гарантиями коммерческой ценности, пригодности для конкретной цели и невыполнения прав. 
    Разработчик не несет ответственности за любые проблемы, ошибки или неполадки, возникшие при использовании данного продукта. Использование продукта осуществляется на ваш собственный риск.
      
###### EN
    The software product provided in this repository is provided "as is" without warranty of any kind, either express or implied, including, but not limited to, the implied warranties of merchantability, fitness for a particular purpose, and non-infringement.
    The developer is not responsible for any problems, errors or malfunctions that occur when using this product. Use of the product is at your own risk.

## Обратная связь
Ниже найдете список ссылок для связи с автором.

| Платформа     | Ссылка                                                                    | Отвечу за |
| ------------- |:-------------------------------------------------------------------------:| --------- |
| Почта         | [Ссылка](mailto:andrewoficial@yandex.ru "Ссылка")                         | 24 часа   |
| LinkedIn      | [Ссылка](https://www.linkedin.com/in/andrey-kantser-126554258/ "Ссылка")  | 3 часа    |
| Telegram      | [Ссылка](https://t.me/function_void "Ссылка")                             | 5 минут   |
