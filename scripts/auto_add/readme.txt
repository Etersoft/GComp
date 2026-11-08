
Скрипт для анализа протокола работы gcomp -d -v, и автоматического добавления/удаления
файлов в репозитарий. Выполняет только команды cvs add и cvs remove - cvs commit нужно
делать вручную.

Скрипт написан в двух вариантах: на Perl'е и на Python'е. Где взять Perl - см. описание 
к correct_dlg. Python можно скачать с http://www.python.org.

Опции командной строки:
   --filter - регулярное выражение. Если имя файла соответствует этому выражению, то он не обрабатывается.

Использование:
\>gcomp -d -v | perl auto_add.pl >>log
или для питона:
\>gcomp -d -v | python auto_add.py >>log


Автор - Диркс Алексей mailto:adirks@ngs.ru

2026: auto_add.py переведён на Python 3 и умеет git: ключ --vcs git
выполняет git add и git rm вместо cvs add и cvs remove. Кодировка протокола
gcomp задаётся ключом --encoding (по умолчанию cp866, как в консоли Windows).
   gcomp -d -v | python3 auto_add.py --vcs git >>log
