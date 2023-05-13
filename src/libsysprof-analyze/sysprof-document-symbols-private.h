/* sysprof-document-symbols-private.h
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

#include "sysprof-document.h"
#include "sysprof-process-info-private.h"
#include "sysprof-symbolizer.h"
#include "sysprof-symbol.h"

G_BEGIN_DECLS

#define SYSPROF_TYPE_DOCUMENT_SYMBOLS (sysprof_document_symbols_get_type())

G_DECLARE_FINAL_TYPE (SysprofDocumentSymbols, sysprof_document_symbols, SYSPROF, DOCUMENT_SYMBOLS, GObject)

void                    _sysprof_document_symbols_new        (SysprofDocument         *document,
                                                              SysprofStrings          *strings,
                                                              SysprofSymbolizer       *symbolizer,
                                                              GHashTable              *pid_to_process_info,
                                                              GCancellable            *cancellable,
                                                              GAsyncReadyCallback      callback,
                                                              gpointer                 user_data);
SysprofDocumentSymbols *_sysprof_document_symbols_new_finish (GAsyncResult            *result,
                                                              GError                 **error);
SysprofSymbol          *_sysprof_document_symbols_lookup     (SysprofDocumentSymbols  *symbols,
                                                              int                      pid,
                                                              SysprofAddressContext    context,
                                                              SysprofAddress           address);

G_END_DECLS
