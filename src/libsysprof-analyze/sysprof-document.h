/* sysprof-document.h
 *
 * Copyright 2023 Christian Hergert <chergert@redhat.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gio/gio.h>

#include <sysprof-capture.h>

#include "sysprof-document-symbols.h"
#include "sysprof-symbolizer.h"

G_BEGIN_DECLS

#define SYSPROF_TYPE_DOCUMENT (sysprof_document_get_type())

SYSPROF_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (SysprofDocument, sysprof_document, SYSPROF, DOCUMENT, GObject)

SYSPROF_AVAILABLE_IN_ALL
SysprofDocument        *sysprof_document_new                (const char           *filename,
                                                             GError              **error);
SYSPROF_AVAILABLE_IN_ALL
SysprofDocument        *sysprof_document_new_from_fd        (int                   capture_fd,
                                                             GError              **error);
SYSPROF_AVAILABLE_IN_ALL
void                    sysprof_document_symbolize_async    (SysprofDocument      *self,
                                                             SysprofSymbolizer    *symbolizer,
                                                             GCancellable         *cancellable,
                                                             GAsyncReadyCallback   callback,
                                                             gpointer              user_data);
SYSPROF_AVAILABLE_IN_ALL
SysprofDocumentSymbols *sysprof_document_symbolize_finish   (SysprofDocument      *self,
                                                             GAsyncResult         *result,
                                                             GError              **error);
SYSPROF_AVAILABLE_IN_ALL
void                    sysprof_document_lookup_file_async  (SysprofDocument      *self,
                                                             const char           *filename,
                                                             GCancellable         *cancellable,
                                                             GAsyncReadyCallback   callback,
                                                             gpointer              user_data);
SYSPROF_AVAILABLE_IN_ALL
GBytes                 *sysprof_document_lookup_file_finish (SysprofDocument      *self,
                                                             GAsyncResult         *result,
                                                             GError              **error);

G_END_DECLS
