// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SERVICETEXT_H
#define SERVICETEXT_H

#include "Strings.h"

/* Views of the service streams of the metadata for reviewing changes; they
   are written next to the streams and never read back. Text is CP1251 with
   CRLF. Each returns 1 with the view, -1 with Error. */

//GUIDData: the history of saves, one GUID a line
int GuidDataToText(const GCString& Data, GCString& Text, GCString& Error);

//TagStream: the "Автор" and "Заставка" pages of the configuration
//properties; Logo and Splash get the pictures as BMP files (empty when
//there is none)
int TagStreamToText(const GCString& Data, GCString& Text, GCString& Logo,
	GCString& Splash, GCString& Error);

#endif
