#!/usr/bin/python3
"""Browse the Main Metadata Stream of 1C:Enterprise 7.7 as a tree.

    mms.view.py [FILE]

FILE is the raw stream (default 'Main Metadata Stream' in the current
directory), CP1251. Enter the number of a branch to open it, 0 to go back.

An example of quick parsing of the brace lists of 1C (also dialogs and the
like); quotes, commas and braces inside strings are not handled.

Author: Fedor Ezeev <fe@alterplast.ru>; ported to Python 3 in 2026.
"""

import ast
import sys


def node_view(node):
    if isinstance(node, str):
        return node + ' (s)'
    if isinstance(node, list):
        if node and isinstance(node[0], str):
            return node[0] + ' (l)'
        return repr(node)[:30] + '...'
    return 'unknown type'


def list_view(tree):
    for num, node in enumerate(tree, 1):
        print(num, node_view(node))
    print(0, 'Exit')


def browse_tree(tree):
    while True:
        list_view(tree)
        try:
            command = int(input())
        except (ValueError, EOFError):
            return
        if command == 0:
            return
        if 1 <= command <= len(tree) and isinstance(tree[command - 1], list):
            browse_tree(tree[command - 1])


def parse(data):
    # The stream may start with its length (1, 3 or 7 bytes): skip to the
    # first brace.
    data = data[data.find(b'{'):]
    text = data.decode('cp1251')

    # Turn the stream into a Python list literal.
    text = text.replace('{', '[').replace('}', ']')
    text = text.replace('\\', '\\\\')  # слэш меняем на двойной
    text = text.replace('\'', '\\\'')  # одинарную кавычку предваряем слэшем
    # Double quotes become single ones only at the bounds of the parameters,
    # that is next to commas and brackets: there may be quotes inside strings.
    text = text.replace('[\"', '[\'')
    text = text.replace(',\"', ',\'')
    text = text.replace('\"]', '\']')
    text = text.replace('\",', '\',')
    return ast.literal_eval(text)


def main():
    if len(sys.argv) > 1 and sys.argv[1] in ('-h', '--help'):
        print(__doc__.strip())
        return
    fname = sys.argv[1] if len(sys.argv) > 1 else 'Main Metadata Stream'
    with open(fname, 'rb') as f:
        tree = parse(f.read())
    browse_tree(tree)


if __name__ == '__main__':
    main()
