/* sysprof-notebook.c
 *
 * Copyright 2019 Christian Hergert <chergert@redhat.com>
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

#define G_LOG_DOMAIN "sysprof-notebook"

#include "config.h"

#include "sysprof-display.h"
#include "sysprof-notebook.h"
#include "sysprof-tab.h"

G_DEFINE_TYPE (SysprofNotebook, sysprof_notebook, GTK_TYPE_NOTEBOOK)

/**
 * sysprof_notebook_new:
 *
 * Create a new #SysprofNotebook.
 *
 * Returns: (transfer full): a newly created #SysprofNotebook
 *
 * Since: 3.34
 */
GtkWidget *
sysprof_notebook_new (void)
{
  return g_object_new (SYSPROF_TYPE_NOTEBOOK, NULL);
}

static void
sysprof_notebook_page_added (GtkNotebook *notebook,
                             GtkWidget   *child,
                             guint        page_num)
{
  g_assert (SYSPROF_IS_NOTEBOOK (notebook));
  g_assert (GTK_IS_WIDGET (child));

  if (SYSPROF_IS_DISPLAY (child))
    {
      GtkWidget *tab = sysprof_tab_new (SYSPROF_DISPLAY (child));

      gtk_notebook_set_tab_label (notebook, child, tab);
      gtk_notebook_set_tab_reorderable (notebook, child, TRUE);
    }

  gtk_notebook_set_show_tabs (notebook,
                              gtk_notebook_get_n_pages (notebook) > 1);
}

static void
sysprof_notebook_page_removed (GtkNotebook *notebook,
                               GtkWidget   *child,
                               guint        page_num)
{
  g_assert (SYSPROF_IS_NOTEBOOK (notebook));
  g_assert (GTK_IS_WIDGET (child));

  if (gtk_widget_in_destruction (GTK_WIDGET (notebook)))
    return;

  if (gtk_notebook_get_n_pages (notebook) == 0)
    {
      child = sysprof_display_new ();
      gtk_container_add (GTK_CONTAINER (notebook), child);
      gtk_widget_show (child);
    }

  gtk_notebook_set_show_tabs (notebook,
                              gtk_notebook_get_n_pages (notebook) > 1);
}

static void
sysprof_notebook_class_init (SysprofNotebookClass *klass)
{
  GtkNotebookClass *notebook_class = GTK_NOTEBOOK_CLASS (klass);

  notebook_class->page_added = sysprof_notebook_page_added;
  notebook_class->page_removed = sysprof_notebook_page_removed;
}

static void
sysprof_notebook_init (SysprofNotebook *self)
{
  gtk_notebook_set_show_border (GTK_NOTEBOOK (self), FALSE);
  gtk_notebook_set_scrollable (GTK_NOTEBOOK (self), TRUE);
  gtk_notebook_popup_enable (GTK_NOTEBOOK (self));
}

void
sysprof_notebook_close_current (SysprofNotebook *self)
{
  gint page;

  g_return_if_fail (SYSPROF_IS_NOTEBOOK (self));

  if ((page = gtk_notebook_get_current_page (GTK_NOTEBOOK (self))) >= 0)
    gtk_widget_destroy (gtk_notebook_get_nth_page (GTK_NOTEBOOK (self), page));
}

static void
find_empty_display_cb (GtkWidget *widget,
                       gpointer   user_data)
{
  GtkWidget **display = user_data;

  g_assert (GTK_IS_WIDGET (widget));
  g_assert (display != NULL);

  if (*display != NULL)
    return;

  if (SYSPROF_IS_DISPLAY (widget) &&
      sysprof_display_is_empty (SYSPROF_DISPLAY (widget)))
    *display = widget;
}

void
sysprof_notebook_open (SysprofNotebook *self,
                       GFile           *file)
{
  GtkWidget *display = NULL;
  gint page;

  g_return_if_fail (SYSPROF_IS_NOTEBOOK (self));
  g_return_if_fail (g_file_is_native (file));

  gtk_container_foreach (GTK_CONTAINER (self),
                         find_empty_display_cb,
                         &display);

  if (display == NULL)
    {

      display = sysprof_display_new ();
      page = gtk_notebook_insert_page (GTK_NOTEBOOK (self), display, NULL, -1);
      gtk_widget_show (display);
    }
  else
    {
      page = gtk_notebook_page_num (GTK_NOTEBOOK (self), display);
    }

  gtk_notebook_set_current_page (GTK_NOTEBOOK (self), page);

  sysprof_display_open (SYSPROF_DISPLAY (display), file);
}

static SysprofDisplay *
sysprof_notebook_get_current (SysprofNotebook *self)
{
  gint page;

  g_assert (SYSPROF_IS_NOTEBOOK (self));

  if ((page = gtk_notebook_get_current_page (GTK_NOTEBOOK (self))) >= 0)
    return SYSPROF_DISPLAY (gtk_notebook_get_nth_page (GTK_NOTEBOOK (self), page));

  return NULL;
}

void
sysprof_notebook_save (SysprofNotebook *self)
{
  SysprofDisplay *display;

  g_return_if_fail (SYSPROF_IS_NOTEBOOK (self));

  if ((display = sysprof_notebook_get_current (self)))
    sysprof_display_save (display);
}