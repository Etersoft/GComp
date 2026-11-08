#!/usr/bin/python3
Version = """
ExtractProc
Version:        2.0
Authors:
    Alexander S. Gordienko <alex-go@vstu.kirov.ru>
    Alexey Dirks <adirks@ngs.ru>
    Python 3 port, 2026
"""
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

import getopt
import os
import re
import sys

# Modules of a GComp 3 tree are UTF-8 with LF by default, CP1251 and CRLF
# with --cp1251 and --eol windows; every file is read and written back in
# its own encoding and line ends.


def read_text(fname):
    data = open(fname, 'rb').read()
    try:
        text, encoding = data.decode('utf-8'), 'utf-8'
    except UnicodeDecodeError:
        text, encoding = data.decode('cp1251'), 'cp1251'
    eol = '\r\n' if '\r\n' in text else '\n'
    return text, encoding, eol


def write_text(fname, text, encoding):
    with open(fname, 'w', encoding=encoding, newline='') as f:
        f.write(text)


#########################################################################
##############      Extract functions into files      ###################
#########################################################################
NAME = r'[0-9a-zA-Zа-яА-ЯёЁ_]+'
headRem     = re.compile(r'^\s*//\*+\s*$', re.I)
bodyRem     = re.compile(r'(^\s*//.*$|^\s*$)', re.I)
forvardProc = re.compile(r'^\s*(?:Функция|Процедура)\s+(?P<name>' + NAME + r')\s*' +
                         r'\([^\)]*\)\s*(?:Экспорт)?\s*(?:Далее)', re.I)
headProc    = re.compile(r'^\s*(?:Функция|Процедура)\s+(?P<name>' + NAME + ')', re.I)
tailProc    = re.compile(r'^\s*(?:КонецФункции|КонецПроцедуры).*$', re.I)
ProcPlace   = re.compile(r'^//###ВынесеноВ (?P<ProcFile>\S+)\s*$', re.I)


def Extract(Dir, ModuleName):
    FullName = os.path.join(Dir, ModuleName)
    ModuleName = os.path.splitext(ModuleName)[0]

    relDir = os.path.relpath(Dir, SrcDir)
    ExtractPath = os.path.normpath(os.path.join(DestDir, relDir, ModuleName))
    relModulePath = os.path.normpath(os.path.join(relDir, ModuleName))

    os.makedirs(ExtractPath, exist_ok=True)

    text, encoding, eol = read_text(FullName)
    Module = text.split(eol)
    newModule = []

    State = 0
    beginScope = 0
    endScope = 0
    procName = ''

    if Verbose > 0:
        print(relModulePath + ':')

    for LineNum, line in enumerate(Module):
        head = ' '.join(Module[LineNum:LineNum + 10])
        if 0 == State:
            tmp = headProc.match(line)
            if forvardProc.match(head) is None and tmp:
                State = 2
                procName = tmp.group('name')
                beginScope = LineNum
            elif headRem.match(line):
                State = 1
                beginScope = LineNum
        elif 1 == State:
            tmp = headProc.match(line)
            if forvardProc.match(head) is None and tmp:
                State = 2
                procName = tmp.group('name')
            elif bodyRem.match(line) is None:
                State = 0
        elif 2 == State:
            if tailProc.match(line):
                State = 0
                newModule += Module[endScope:beginScope]
                newModule.append('//###ВынесеноВ ' + procName + '.txt')
                endScope = LineNum + 1

                if Verbose > 1:
                    print('    .' + procName)
                write_text(os.path.join(ExtractPath, procName + '.txt'),
                           eol.join(Module[beginScope:endScope]), encoding)

    if Verbose > 1:
        print()

    newModule += Module[endScope:]
    write_text(os.path.join(ExtractPath, ModuleName + '.1ss'), eol.join(newModule), encoding)


#########################################################################
############## Assemble separate functions into single module  ##########
#########################################################################
def Assemble(Dir, ModuleName):
    relDir = os.path.relpath(Dir, SrcDir)

    FullModuleName = os.path.join(Dir, ModuleName)
    relName = os.path.normpath(os.path.join(relDir, ModuleName))
    ModuleName = os.path.splitext(ModuleName)[0]

    SkeletonDir = os.path.join(DestDir, relDir, ModuleName)
    SkeletonName = os.path.join(SkeletonDir, ModuleName + '.1ss')

    if not os.path.exists(SkeletonName):
        return

    if Verbose > 0:
        print(relName + ':')

    text, encoding, eol = read_text(SkeletonName)
    Module = []
    ToWrite = 0
    for line in text.split(eol):
        res = ProcPlace.match(line)
        if res:
            ProcFile = os.path.join(SkeletonDir, res.group('ProcFile'))
            Module.append(read_text(ProcFile)[0])
            ToWrite = 1
            if Verbose > 1:
                print('    .' + res.group('ProcFile'))
        else:
            Module.append(line)

    if Verbose > 1:
        print()

    # the skeleton keeps the lines between the procedures as they were, so
    # the module is assembled back byte for byte
    if ToWrite:
        write_text(FullModuleName, eol.join(Module), encoding)


def walk(action):
    for dirpath, dirnames, filenames in os.walk(SrcDir):
        dirnames.sort()
        for name in sorted(filenames):
            if os.path.splitext(name)[1].lower() == '.1s':
                action(dirpath, name)


def main():
    global SrcDir, DestDir, Verbose

    try:
        opts, args = getopt.getopt(sys.argv[1:],
                                   'hEVemp:f:b:sv',
                                   ['help', 'examples',
                                    'version', 'extract',
                                    'make', 'extract_path=',
                                    'file=', 'base_path=',
                                    'silent', 'verbose'])
    except getopt.GetoptError:
        usage()
        sys.exit(2)

    Action = 0
    SrcDir = ''
    DestDir = ''
    fnModule = ''
    ProcessAll = 1
    Verbose = 1

    for o, a in opts:
        if o in ('-h', '--help'):
            usage()
            sys.exit()
        elif o in ('-E', '--examples'):
            examples()
        elif o in ('-V', '--version'):
            version()
        elif o in ('-e', '--extract'):
            Action = 1
        elif o in ('-m', '--make'):
            Action = 2
        elif o in ('-p', '--extract_path'):
            DestDir = os.path.abspath(a)
        elif o in ('-f', '--file'):
            fnModule = os.path.abspath(a)
            ProcessAll = 0
        elif o in ('-b', '--base_path'):
            SrcDir = os.path.abspath(a)
            ProcessAll = 1
        elif o in ('-s', '--silent'):
            if Verbose < 2:
                Verbose = 0
        elif o in ('-v', '--verbose'):
            Verbose = 2

    if ProcessAll == 0:
        (SrcDir, fnModule) = os.path.split(fnModule)

    if SrcDir == '':
        SrcDir = os.path.abspath('Src')
    if DestDir == '':
        DestDir = SrcDir

    if Action == 1:
        if ProcessAll == 1:
            walk(Extract)
        else:
            Extract(SrcDir, fnModule)
    elif Action == 2:
        if ProcessAll == 1:
            walk(Assemble)
        else:
            Assemble(SrcDir, fnModule)
    else:
        usage()
        sys.exit(2)


def usage():
    print(r"""
Запуск: ExtractProc.py {-h|-E|-V|-e|-m}
                       [-b <base_path> | [-f <file>] [-p <extract_path>]]
                       [-s|-v]

Аргументы:
    -h | --help         - эта помощь
    -E | --examples     - показать примеры использования
    -V | --version      - отображение версии скрипта и выход
    -e | --extract      - извлечь процедуры/функции из модуля
    -m | --make         - внести процедуры/функции в модуль
    -b | --base_path    - путь к распакованной gcomp конфигурации
                            (обрабатываются все *.1s)
    -f | --file         - файл модуля
    -p | --extract_path - путь для извлечения
    -s | --silent       - "тихий" режим (данный режим перекрывается --verbose)
    -v | --verbose      - расширенный вывод
""")


def examples():
    print(r"""
Примеры:

  Извлечь модуль 'Src/Документы/Аккредитив/МодульФормы.1s' в папку по
  умолчанию ('Src/Документы/Аккредитив/МодульФормы/'):
    ExtractProc.py -e -f Src/Документы/Аккредитив/МодульФормы.1s

  Извлечь модуль 'Src/ГлобальныйМодуль.1s' в папку 'Src/ГлобальныйМодуль/':
    ExtractProc.py -e -f Src/ГлобальныйМодуль.1s -p Src/ГлобальныйМодуль

  Извлечь все модули распакованной конфигурации расположенной в папке по
  умолчанию ('Src/'):
    ExtractProc.py -e

  Извлечь все модули распакованной конфигурации расположенной в
  папке 'SrcDir/':
    ExtractProc.py -e -b SrcDir

  Извлечь все модули распакованной конфигурации расположенной в
  папке 'SrcDir/' в папку /tmp/SrcDirExt/:
    ExtractProc.py -e -b SrcDir -p /tmp/SrcDirExt

  Внесение делается аналогично:
    ExtractProc.py -m
""")
    sys.exit()


def version():
    print(Version)
    sys.exit()


if __name__ == '__main__':
    main()
