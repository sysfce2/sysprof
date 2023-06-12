/* sysprof-callgraph-view-private.h
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

#include "sysprof-callgraph-view.h"

G_BEGIN_DECLS

#define SYSPROF_CALLGRAPH_VIEW_GET_CLASS(instance) G_TYPE_INSTANCE_GET_CLASS(instance, SYSPROF_TYPE_CALLGRAPH_VIEW, SysprofCallgraphViewClass)

struct _SysprofCallgraphView
{
  GtkWidget parent_instance;

  SysprofDocument *document;
  SysprofCallgraph *callgraph;
  GListModel *traceables;

  GtkColumnView *callers_column_view;
  GtkColumnView *descendants_column_view;
  GtkColumnView *functions_column_view;
  GtkColumnView *traceables_column_view;
  GtkCustomSorter *descendants_name_sorter;
  GtkCustomSorter *functions_name_sorter;
  GtkScrolledWindow *scrolled_window;
  GtkWidget *paned;

  GCancellable *cancellable;

  guint reload_source;
};

struct _SysprofCallgraphViewClass
{
  GtkWidgetClass parent_class;

  gsize augment_size;
  void (*augment_func) (SysprofCallgraph     *callgraph,
                        SysprofCallgraphNode *node,
                        SysprofDocumentFrame *frame,
                        gpointer              user_data);

  void (*load) (SysprofCallgraphView *self,
                SysprofCallgraph     *callgraph);
};

G_END_DECLS
