// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef MOXELTEXT_H
#define MOXELTEXT_H

#include "Strings.h"

/* Markdown view of a moxcel table (the bytes of a .mxl), CP1251 with CRLF.
   Returns 1 with the text, 0 for a table without content (no text is
   needed), -1 with Error when the table cannot be read. */
int MoxelToText(const GCString& Data, GCString& Text, GCString& Error);

#endif
