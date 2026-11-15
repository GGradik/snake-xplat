# CWSDPMI r7

DPMI-сервер для программ DJGPP (автор Charles W. Sandmann, условия
распространения — в `cwsdpmi.doc`). Без DPMI-сервера 32-битная программа
DJGPP не запустится в DOS/DOSBox («Load error: no DPMI»).

Используется `CWSDSTUB.EXE` — загрузчик со встроенным CWSDPMI. При сборке
(`make dos`) он склеивается с COFF-образом игры, и получается один
самодостаточный `SNAKE.EXE` (способ описан в `cwsdpmi.doc`).

Файлы не изменялись. Источник: зеркало https://github.com/jayschwa/cwsdpmi
(исходники и бинарные файлы CWSDPMI r7, оригинал — http://sandmann.dyndns.info/cwsdpmi/).
