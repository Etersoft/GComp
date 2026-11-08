#!/usr/bin/python3
"""Show which pictures of the gallery the forms of a configuration use.

    show_all_pics.py SRC

SRC is a configuration decompiled by GComp 3. 1C does not keep the
referential integrity of its picture gallery, so the script lists every
form with pictures and the pictures it uses, then the used, unused and
lost pictures (referred to by a form but missing from PictureGallery).

The form element property Пиктограмма and the form property
ИдентификаторКартинки hold the picture ID; the gallery files are named
__Picture.<ID in hex>.<ext>.

Author: Fedor Ezeev <fe@alterplast.ru>, for the brace dialogs of GComp
2.0.9; rewritten for the YAML forms of GComp 3 and Python 3 in 2026.
"""

import os
import re
import sys

RE_ITEM = re.compile(r'^\s*- (?P<cls>[0-9A-Za-z]+):\s*(?P<ident>.*)$')
RE_PICTURE = re.compile(r"^\s*(?P<key>Пиктограмма|ИдентификаторКартинки):\s*'?(?P<id>[0-9A-Fa-fx]+)'?\s*$")
RE_CAPTION = re.compile(r'^\s*Заголовок:\s*(?P<text>.*)$')


def picture_id(value):
    return int(value, 0)


def collect_gallery(src):
    result = set()
    gallery = os.path.join(src, 'PictureGallery')
    if os.path.isdir(gallery):
        for name in os.listdir(gallery):
            m = re.match(r'__Picture\.([0-9A-Fa-f]+)', name)
            if m:
                result.add(int(m.group(1), 16))
    return result


def parse_form(fname):
    """Yield (where, picture ID) for the pictures of a form."""
    item = None
    caption = ''
    for line in open(fname, encoding='utf-8', errors='replace'):
        line = line.rstrip('\n')
        m = RE_ITEM.match(line)
        if m:
            item = (m.group('cls'), m.group('ident').strip())
            caption = ''
            continue
        m = RE_CAPTION.match(line)
        if m and item:
            caption = m.group('text').strip()
            continue
        m = RE_PICTURE.match(line)
        if m:
            value = picture_id(m.group('id'))
            if not value:
                continue
            if m.group('key') == 'ИдентификаторКартинки' or not item:
                yield 'Форма', value
            else:
                yield '%s (%s, %s)' % (item[0], caption or '""', item[1] or '""'), value


def show(ids, note):
    print('==============================')
    if not ids:
        print('List of %s pics is empty' % note)
    else:
        print('List of %s pics: %3i items' % (note, len(ids)))
    for i in sorted(ids):
        print('%08x' % i)


def main():
    if len(sys.argv) != 2 or sys.argv[1].startswith('-'):
        print(__doc__.strip(), file=sys.stderr)
        return 2
    src = sys.argv[1]
    gallery = collect_gallery(src)
    used = set()
    for dirpath, dirnames, filenames in os.walk(src):
        dirnames.sort()
        for name in sorted(filenames):
            if not name.endswith('.yaml'):
                continue
            fname = os.path.join(dirpath, name)
            pictures = list(parse_form(fname))
            if not pictures:
                continue
            print(os.path.relpath(fname, src))
            for where, value in pictures:
                used.add(value)
                lost = '' if value in gallery else '  ---  !!! LOST !!!'
                print('\t%s: %08x%s' % (where, value, lost))
            print()
    show(used, 'used')
    show(gallery - used, 'unused')
    show(used - gallery, 'lost')
    return 0


if __name__ == '__main__':
    sys.exit(main())
