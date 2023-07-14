/* sysprof-marks-section.c
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

#include "config.h"

#include <sysprof-gtk.h>

#include "sysprof-marks-section.h"

struct _SysprofMarksSection
{
  SysprofSection parent_instance;

  SysprofMarkChart *mark_chart;
  SysprofMarkTable *mark_table;
};

G_DEFINE_FINAL_TYPE (SysprofMarksSection, sysprof_marks_section, SYSPROF_TYPE_SECTION)

static char *
format_number (gpointer unused,
               guint    number)
{
  if (number == 0)
    return NULL;
  return g_strdup_printf ("%'u", number);
}

static void
sysprof_marks_section_dispose (GObject *object)
{
  SysprofMarksSection *self = (SysprofMarksSection *)object;

  gtk_widget_dispose_template (GTK_WIDGET (self), SYSPROF_TYPE_MARKS_SECTION);

  G_OBJECT_CLASS (sysprof_marks_section_parent_class)->dispose (object);
}

static void
sysprof_marks_section_class_init (SysprofMarksSectionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = sysprof_marks_section_dispose;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/sysprof/sysprof-marks-section.ui");
  gtk_widget_class_bind_template_child (widget_class, SysprofMarksSection, mark_chart);
  gtk_widget_class_bind_template_child (widget_class, SysprofMarksSection, mark_table);
  gtk_widget_class_bind_template_callback (widget_class, format_number);

  g_type_ensure (SYSPROF_TYPE_CHART);
  g_type_ensure (SYSPROF_TYPE_DOCUMENT_MARK);
  g_type_ensure (SYSPROF_TYPE_MARK_CHART);
  g_type_ensure (SYSPROF_TYPE_MARK_TABLE);
  g_type_ensure (SYSPROF_TYPE_SESSION_MODEL);
  g_type_ensure (SYSPROF_TYPE_SESSION_MODEL_ITEM);
  g_type_ensure (SYSPROF_TYPE_TIME_SERIES);
  g_type_ensure (SYSPROF_TYPE_TIME_SPAN_LAYER);
}

static void
sysprof_marks_section_init (SysprofMarksSection *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
