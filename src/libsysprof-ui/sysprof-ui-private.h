/* sysprof-ui-private.h
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

#pragma once

#include "sysprof-marks-view.h"
#include "sysprof-visualizer-view.h"

G_BEGIN_DECLS

void _sysprof_marks_view_set_hadjustment      (SysprofMarksView      *self,
                                               GtkAdjustment         *hadjustment);
void _sysprof_visualizer_view_set_hadjustment (SysprofVisualizerView *self,
                                               GtkAdjustment         *hadjustment);
void _sysprof_rounded_rectangle               (cairo_t               *cr,
                                               const GdkRectangle    *rect,
                                               gint                   x_radius,
                                               gint                   y_radius);

G_END_DECLS
