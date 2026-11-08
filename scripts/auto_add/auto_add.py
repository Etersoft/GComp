#!/usr/bin/python3
"""Add new files to and remove deleted files from a repository after
decompiling with GComp.

    gcomp -d -v | auto_add.py [--filter REGEXP] [--vcs cvs|git] [--encoding ENC] >>log

The script reads the log of `gcomp -d -v` from stdin. For each new file
(prefix 'NEW:') it runs `cvs add` (or `git add`), adding the parent
directories first for CVS; for each deleted file (prefix 'DEL:') it runs
`cvs remove` (or `git rm`). Other lines are copied to stdout. Commit is
left to the user.

Author: Alexey Dirks <adirks@ngs.ru>; ported to Python 3 in 2026.
"""

import argparse
import os
import re
import subprocess
import sys
import time

SESSION_SPLITTER = "-" * 97

RE_NEW = re.compile(r"^NEW:\s*'*(.+?)'*\s*$", re.I)
RE_DEL = re.compile(r"^DEL:\s*'*(.+?)'*\s*$", re.I)


def parse_line(line):
    res = RE_NEW.search(line)
    if res:
        return ('add', 'ADD', res.group(1))
    res = RE_DEL.search(line)
    if res:
        return ('remove', 'DEL', res.group(1))
    return ('', '', '')


def do_command(working_dir, args):
    print('\\%s>%s' % (working_dir, ' '.join(args)))
    res = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                         universal_newlines=True, errors='replace')
    for line in res.stdout.splitlines():
        print('\t%s' % line)


def split_path(path):
    parts = []
    while path:
        path, part = os.path.split(path)
        if part:
            parts.append(part)
        elif path:
            parts.append(path)
            break
    parts.reverse()
    return parts


def vcs_command(vcs, command):
    if vcs == 'git':
        return ['git', 'add'] if command == 'add' else ['git', 'rm', '--quiet']
    return ['cvs', command]


def process(fname, command, vcs):
    """Run the command in the first directory of the path (the tree), adding
    the directories between it and the file for CVS."""
    parts = split_path(fname.replace('\\', '/'))
    working_dir = parts[0]
    cwd = os.getcwd()
    os.chdir(working_dir)
    try:
        print(os.getcwd())
        path = ''
        if vcs == 'cvs' and command == 'add':
            for part in parts[1:-1]:
                path = os.path.join(path, part)
                print('ADD Dir: %s' % path)
                print('ADD Dir: %s' % path, file=sys.stderr)
                do_command(working_dir, ['cvs', 'add', path])
        do_command(working_dir, vcs_command(vcs, command) + [os.path.join(*parts[1:])])
    finally:
        os.chdir(cwd)


def analyze(stream, re_filter, vcs):
    session = False
    for line in stream:
        line = line.rstrip('\r\n')
        command, msg, fname = parse_line(line)
        if not command:
            print(line)
            continue
        if re_filter and re_filter.search(fname):
            print("File '%s' skipped by filter" % fname)
            continue
        if not session:
            print(time.strftime('%Y.%m.%d %H:%M:%S'))
            print(SESSION_SPLITTER)
            session = True
        print('%s %s' % (msg, fname))
        process(fname, command, vcs)
    if session:
        print(SESSION_SPLITTER)


def main():
    parser = argparse.ArgumentParser(
        description='Add new and remove deleted files after gcomp -d -v')
    parser.add_argument('-f', '--filter', help='skip the files matching this regular expression')
    parser.add_argument('--vcs', choices=('cvs', 'git'), default='cvs',
                        help='version control system (default cvs)')
    parser.add_argument('--encoding', default='cp866',
                        help='encoding of the gcomp log (default cp866, as the Windows console)')
    args = parser.parse_args()

    re_filter = re.compile(args.filter, re.I) if args.filter else None
    stream = open(sys.stdin.fileno(), encoding=args.encoding, errors='replace', closefd=False)
    analyze(stream, re_filter, args.vcs)


if __name__ == '__main__':
    main()
