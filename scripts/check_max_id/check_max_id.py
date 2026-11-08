#!/usr/bin/python3
"""Check that the next metadata ID of a decompiled configuration is in the
range given to the developer.

Several developers working on one configuration can avoid conflicts of
internal IDs: the project administrator keeps a file with lines

    <user name> <min ID> <max ID>

(the last line of a user wins; lines starting with ;, # or // are
comments). The script reads `Next ID` from ИдентификаторыКонфигурации.yaml
of the tree and fails when it is out of the user's range.

Author: Alexey Dirks <adirks@ngs.ru>; ported to Python 3 and GComp 3 in 2026.
"""

import argparse
import getpass
import os
import re
import sys

METADATA_IDS_FILE = 'ИдентификаторыКонфигурации.yaml'


def fail(message):
    print(message)
    sys.exit(1)


def read_lines(fname):
    try:
        with open(fname, encoding='utf-8', errors='replace') as f:
            return f.read().splitlines()
    except OSError as e:
        fail('Can not open file %s: %s' % (fname, e.strerror))


def get_login_name(working_dir):
    """The user of the CVS checkout, or the login name."""
    fname = os.path.join(working_dir, 'CVS', 'Root')
    if not os.path.exists(fname):
        return getpass.getuser()
    # :protocol:user@host:cvs_repository_path_on_host
    re_name = re.compile(r'.*:(?P<uname>\S+)@(\S+):.*')
    user_name = ''
    for line in read_lines(fname):
        res = re_name.search(line)
        if res:
            user_name = res.group('uname')
    return user_name or getpass.getuser()


def find_current_range(fname, user_name):
    re_comment = re.compile(r'^\s*([;#]|//)')
    re_range = re.compile(r'^\s*(?P<uname>\S+)\s+(?P<min_id>\d+)\s+(?P<max_id>\d+)\s*$')
    min_id = max_id = 0
    for line in read_lines(fname):
        if re_comment.search(line):
            continue
        res = re_range.search(line)
        if res and res.group('uname').upper() == user_name.upper():
            min_id = int(res.group('min_id'))
            max_id = int(res.group('max_id'))
    if not min_id or not max_id:
        fail("ERROR: ID's range not found for user %s" % user_name)
    return min_id, max_id


def get_next_metadata_id(working_dir):
    fname = os.path.join(working_dir, METADATA_IDS_FILE)
    re_next_id = re.compile(r'^\s*Next ID\s*:\s*(?P<id>\d+)\s*$')
    for line in read_lines(fname):
        res = re_next_id.search(line)
        if res:
            return int(res.group('id'))
    fail('Can not determine max metadata ID. May be file format had changed?')


def main():
    parser = argparse.ArgumentParser(
        description='Check the next metadata ID against the range of the user')
    parser.add_argument('-d', '-D', '--dir', default='SRC',
                        help='decompiled tree (default SRC)')
    parser.add_argument('-t', '-w', '--threshold', type=int, default=100,
                        help='warn when this many IDs are left (default 100)')
    parser.add_argument('-f', '--range-file', default=os.path.join('..', 'id_settings.txt'),
                        help='file with the ranges, relative to the tree (default ../id_settings.txt)')
    parser.add_argument('-u', '--user',
                        help='user name (default: the user of <dir>/CVS/Root, else the login name)')
    args = parser.parse_args()

    user_name = args.user or get_login_name(args.dir)
    min_id, max_id = find_current_range(os.path.join(args.dir, args.range_file), user_name)
    next_id = get_next_metadata_id(args.dir)

    print('%s: %i, %i; max MD ID = %i' % (user_name, min_id, max_id, next_id))
    if next_id < min_id:
        print('ERROR: Current metadata ID is out of range! Commit impossible!')
        fail('\tID = %i < min ID = %i' % (next_id, min_id))
    if next_id >= max_id:
        print('ERROR: Current metadata ID is out of range! Commit impossible!')
        fail('\tID = %i >= max ID = %i' % (next_id, max_id))
    if next_id + args.threshold >= max_id:
        print("WARNING: Current metadata ID is near the top! "
              "You need to get new ID's range from project administrator!")
        print('\tID = %i; max ID = %i' % (next_id, max_id))


if __name__ == '__main__':
    main()
