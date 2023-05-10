# ESTC-project

# Сборка
<pre>
make dfu [ESTC_USB_CLI_ENABLED=1] [SDK_ROOT=/path/to/sdk]
ESTC_USB_CLI_ENABLED - флаг, отвечающий за включения CLI команд при сборке. 1 по умолчанию
SDK_ROOT - месторасположение esl_nsdk
</pre>
Makefile расположен в /armgcc директории


# Функционал:

Изменение цвета LED2 с сохранением цвета в флеш память.

<h2>Управление кнопкой</h2>
Двойной клик переключает режим изменения цвета.
<br></br>
<ul>
Режимы изменения цвета:
  <li>NO_INPUT: цвет не меняется. Измененный цвет записываются на данном шаге в постоянную память.</li>
  <li>HUE_MODIFICATION: изменение Hue при единичном нажатии на кнопку</li>
  <li>SATURATION_MODIFICATION: изменение Saturation при единичном нажатии на кнопку</li>
  <li>BRIGHTNESS_MODIFICATION: изменение Brightness при единичном нажатии на кнопку</li>
</ul>

<h2>CLI команды</h2>
Работает при выставленном флаге ESTC_USB_CLI_ENABLED=1. Подключение осуществляется с помощью picocom: picocom <USB порт>
<br></br>
Команды:
<pre>
help - Выдает подсказки по всем существующим командам

RGB <red> <green> <blue> - Применяет заданный цвет к LED2
HSV <hue> <saturation> <vue> - Применяет заданный цвет к LED2

add_rgb_color <red> <green> <blue> <color_name> - Сохраняет заданный цвет в постоянную память. Если количество сохраненных цветов равняется 10, то последний сохраненный цвет отбрасывается
add_current_color <color_name> - Сохраняет текущий цвет в постоянную память. Если количество сохраненных цветов равняется 10, то последний сохраненный цвет отбрасывается
apply_color <color_name> - Применяет цвет с именем color_name к LED2
del_color <color_name> - Удаляет цвет color_name из памяти
</pre>

<h2>BLE Interface</h2
Название девайса: BLE LED Service

Присутсвует 2 характеристики: характеристика для чтения текущего цвета и для записи цвета.
<br></br>
Значение характеристики - число в 3 байта. Первый байт отвечает за Red составлюущую, второй за Green, а третий за Blue.
<br></br> 
При изменении цвета (не важно, если цвет изменили через CLI или через кнопку, а может и через BLE сервис) отправляется нотификация, если был включен CCCD в приложении NRF Connect
<br></br>
Для изменения цвета отправляется 3 байтовое число через приложение NRF Connect. Перед этим происходит процесс pairing\`а. Bonding\`а не происходит из-за возможности конфликтов модуля modules/fs/fs.h и NRF\`овского fds.h
