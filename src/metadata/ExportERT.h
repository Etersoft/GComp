// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef EXPORTERT_H
#define EXPORTERT_H

class CMMS;
class CStorage;

/* --ert: every report and data processor of the configuration decompiled
   into Dir is also written as <object dir>\<name>.ert, as the Configurator
   saves it as an external report; Root is the compound of the MD,
   TypedText its open storage TypedText. */
void ExportERTs(CMMS& MMS, GCString& Dir, CStorage& Root, CStorage& TypedText);

#endif
