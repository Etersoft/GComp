#!/usr/bin/python3
"""Convert a 1C:Enterprise 7.7 moxcel table (.mxl) to XML.

    mxl2xml.py FILE.mxl [FILE.mxl ...]

Writes FILE.mxl.xml (UTF-8) next to each table. Pictures and OLE objects
are skipped. The format is described in docs/moxcel.md.

The parser knows only part of the format: tables of version 6 (MOXCEL
header) are mostly read, older (7.5) and newer ones mostly are not. A
table it cannot read is reported and skipped; the exit code is then 1.

Originally written for Python 2 and GComp 2.2.15 (moxcel-1b1, 2005);
ported to Python 3 in 2026.
"""

import os
import struct
import sys
from struct import unpack
from xml.sax.saxutils import XMLGenerator

ENCODING = "cp1251"


def mxl2xml(fn):
    with open(fn, "rb") as fi, open(fn + ".xml", "wb") as fo:
        xml = XMLGenerator(fo, "utf-8")
        xml.startDocument()
        _table(fi, xml)
        xml.endDocument()


def convert(fn):
    """Convert one table; on a structure this parser does not know, report
    it and remove the partial XML."""
    try:
        mxl2xml(fn)
        return True
    except (struct.error, ValueError) as e:
        print("%s: не разобран: %s" % (fn, e), file=sys.stderr)
        try:
            os.remove(fn + ".xml")
        except OSError:
            pass
        return False


def _text(data):
    # control characters are not allowed in XML 1.0: write them as \xNN
    text = data.decode(ENCODING, "replace")
    return "".join(c if c >= " " or c in "\t\n\r" else "\\x%02x" % ord(c)
                   for c in text)


def _debug(name, data):
    if data:
        print("%s: %r" % (name, data), file=sys.stderr)


def _write_element(xml, name, value):
    xml.startElement(name, {})
    xml.characters(value)
    xml.endElement(name)


def _numbers(fi):
    size, = unpack("=H", fi.read(2))
    return unpack("=%di" % size, fi.read(size * 4)) if size else ()


def _count(fi, numbs):
    size, = unpack("=H", fi.read(2))
    if size and size != len(numbs):
        raise ValueError("%d numbers for %d items" % (len(numbs), size))
    return size


def _table(fi, xml):
    # root - table
    fi.read(0x37)
    xml.startElement("table", {})
    _fonts(fi, xml)
    _debug("_unk1", fi.read(4))
    _page_hdrs(fi, xml)
    _cols(fi, xml)
    _rows(fi, xml)
    _objs(fi, xml)
    _merge(fi, xml)
    _sects(fi, xml, "v_sects", "v_sect")
    _sects(fi, xml, "h_sects", "h_sect")
    _pagefeeds(fi, xml, "PageFeedsVert", "PageFeedVert")
    _pagefeeds(fi, xml, "PageFeedsHor", "PageFeedHor")
    _names(fi, xml)
    xml.endElement("table")
    _debug("_rest", fi.read())


def _fonts(fi, xml):
    numbs = _numbers(fi)
    size = _count(fi, numbs)
    if size:
        xml.startElement("fonts", {"size": str(size)})
        for i in range(size):
            xml.startElement("font", {"id": str(numbs[i])})
            _font(fi, xml)
            xml.endElement("font")
        xml.endElement("fonts")


def _page_hdrs(fi, xml):
    xml.startElement("page_hdr_top", {})
    _cell(fi, xml, 0, "BeginFlag", "BeginNumber")
    xml.endElement("page_hdr_top")
    xml.startElement("page_hdr_bottom", {})
    _cell(fi, xml, 0, "BeginFlag", "BeginNumber")
    xml.endElement("page_hdr_bottom")


def _font(fi, xml):
    buf = fi.read(60)
    _write_element(xml, "name", _text(buf[0x1c:].split(b"\x00", 1)[0]))


def _cols(fi, xml):
    numbs = _numbers(fi)
    size = _count(fi, numbs)
    if size:
        xml.startElement("cols", {"size": str(size)})
        for i in range(size):
            xml.startElement("col", {"id": str(numbs[i])})
            _cell(fi, xml, 0, "ПоложНаСтраницеГориз", "ColumnWidth")
            xml.endElement("col")
        xml.endElement("cols")


def _rows(fi, xml):
    numbs = _numbers(fi)
    size = _count(fi, numbs)
    if size:
        xml.startElement("rows", {"size": str(size)})
        for i in range(size):
            xml.startElement("row", {"id": str(numbs[i])})
            _cell(fi, xml, 0, "RowHeight", "ПоложНаСтраницеВерт")
            _cells(fi, xml)
            xml.endElement("row")
        xml.endElement("rows")


def _cells(fi, xml):
    numbs = _numbers(fi)
    size = _count(fi, numbs)
    if size:
        xml.startElement("cols", {"size": str(size)})
        for i in range(size):
            _cell(fi, xml, numbs[i])
        xml.endElement("cols")


CELL_PROPS = [
    # flag byte, flag bit, value byte, name
    (0x00, 0x01, 0x08, "Font"),
    (0x00, 0x02, 0x0a, "FontSize"),
    (0x00, 0x04, 0x0c, "Bold"),
    (0x00, 0x08, 0x0d, "Italic"),
    (0x00, 0x10, 0x0e, "Underline"),
    (0x01, 0x10, 0x0f, "HorizontalAlign"),
    (0x01, 0x20, 0x10, "VerticalAlign"),
    (0x02, 0x01, 0x11, "Узор"),
    (0x00, 0x20, 0x12, "LeftBorder"),
    (0x00, 0x40, 0x13, "TopBorder"),
    (0x00, 0x80, 0x14, "RightBorder"),
    (0x01, 0x01, 0x15, "BottomBorder"),
    (0x02, 0x02, 0x16, "Цвет_узора"),
    (0x01, 0x02, 0x17, "BorderColor"),
    (0x01, 0x40, 0x18, "TextColor"),
    (0x01, 0x80, 0x19, "BackgroundColor"),
    (0x02, 0x04, 0x1a, "Control"),
    (0x02, 0x08, 0x1b, "Тип"),
    (0x02, 0x10, 0x1c, "Protection"),
]


def _cell(fi, xml, id, share1="Share1", share2="Share2"):
    xml.startElement("cell", {"id": str(id)})
    buf = fi.read(30)
    for addr_f, flag, addr_v, name in CELL_PROPS:
        if buf[addr_f] & flag:
            _write_element(xml, name, str(buf[addr_v]))
    # row height/page header, col width/page header
    if buf[0x01] & 0x04:
        _write_element(xml, share1, str(unpack("=H", buf[0x04:0x06])[0]))
    if buf[0x01] & 0x08:
        _write_element(xml, share2, str(unpack("=H", buf[0x06:0x08])[0]))
    # text, detail
    if buf[3] & 0x80:
        size, = unpack("=B", fi.read(1))
        _write_element(xml, "Text", _text(fi.read(size)))
    if buf[3] & 0x40:
        size, = unpack("=B", fi.read(1))
        _write_element(xml, "Details", _text(fi.read(size)))
    xml.endElement("cell")


OBJ_FIELDS = ["ТипОбъекта", "КолНач", "СтрНач", "XНач", "YНач",
              "КолКон", "СтрКон", "XКон", "YКон", "Уровень"]


def _objs(fi, xml):
    size, = unpack("=H", fi.read(2))
    if size:
        xml.startElement("objs", {"size": str(size)})
        for i in range(size):
            xml.startElement("obj", {"id": str(i + 1)})
            _cell(fi, xml, 0)
            tmp = unpack("=10i", fi.read(40))
            for name, value in zip(OBJ_FIELDS, tmp):
                _write_element(xml, name, str(value))
            if tmp[0] == 5:
                # picture
                tmp2 = unpack("=2i", fi.read(8))
                fi.read(tmp2[1])
                _write_element(xml, "Рисунок", "не разобран")
            if tmp[0] == 4:
                # OLE: type name, then the object
                tmp2 = unpack("=iH", fi.read(6))
                fi.read(tmp2[1])
                tmp3 = unpack("=3iH2i", fi.read(22))
                fi.read(tmp3[5])
                _write_element(xml, "ОЛЕ", "не разобран")
            xml.endElement("obj")
        xml.endElement("objs")


def _merge(fi, xml):
    size, = unpack("=H", fi.read(2))
    if size:
        xml.startElement("merge", {"size": str(size)})
        for i in range(size):
            begin_c, begin_r, end_c, end_r = unpack("=4i", fi.read(16))
            xml.startElement("merge_el", {"id": str(i + 1)})
            _write_element(xml, "begin_col", str(begin_c))
            _write_element(xml, "begin_row", str(begin_r))
            _write_element(xml, "end_col", str(end_c))
            _write_element(xml, "end_row", str(end_r))
            xml.endElement("merge_el")
        xml.endElement("merge")


def _sects(fi, xml, main, child):
    size, = unpack("=H", fi.read(2))
    if size:
        xml.startElement(main, {"size": str(size)})
        for i in range(size):
            begin, end, parent, _unk, size_name = unpack("=iiHHB", fi.read(13))
            name = _text(fi.read(size_name))
            xml.startElement(child, {"id": str(i + 1)})
            _write_element(xml, "begin", str(begin))
            _write_element(xml, "end", str(end))
            _write_element(xml, "parent", str(parent))
            _write_element(xml, "name", name)
            xml.endElement(child)
        xml.endElement(main)


def _pagefeeds(fi, xml, main, child):
    size, = unpack("=H", fi.read(2))
    if size:
        numbs = unpack("=%di" % size, fi.read(size * 4))
        xml.startElement(main, {"size": str(size)})
        for i in range(size):
            xml.startElement(child, {"id": str(i + 1)})
            xml.characters(str(numbs[i]))
            xml.endElement(child)
        xml.endElement(main)


def _names(fi, xml):
    size, = unpack("=i", fi.read(4))
    if size:
        xml.startElement("names", {"size": str(size)})
        for i in range(size):
            xml.startElement("name", {"id": str(i + 1)})
            size_nm, = unpack("=B", fi.read(1))
            _write_element(xml, "NameText", _text(fi.read(size_nm)))
            values = unpack("=7i", fi.read(28))
            for name, value in zip(["_unk1", "_unk2", "_unk3", "BeginCol",
                                    "BeginRow", "EndCol", "EndRow"], values):
                _write_element(xml, name, str(value))
            xml.endElement("name")
        xml.endElement("names")


def main(argv):
    if len(argv) < 2:
        print(__doc__.strip(), file=sys.stderr)
        return 2
    ok = True
    for fn in argv[1:]:
        ok = convert(fn) and ok
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
