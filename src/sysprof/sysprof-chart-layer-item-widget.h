/* sysprof-chart-layer-item-widget.h
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

#include "sysprof-chart-layer.h"
#include "sysprof-chart-layer-item.h"

G_BEGIN_DECLS

#define SYSPROF_TYPE_CHART_LAYER_ITEM_WIDGET (sysprof_chart_layer_item_widget_get_type())

G_DECLARE_FINAL_TYPE (SysprofChartLayerItemWidget, sysprof_chart_layer_item_widget, SYSPROF, CHART_LAYER_ITEM_WIDGET, SysprofChartLayer)

SysprofChartLayerItemWidget *sysprof_chart_layer_item_widget_new (SysprofChartLayerItem *item);

G_END_DECLS
