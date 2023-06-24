/* sysprof-column-layer.c
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

#include "sysprof-axis.h"
#include "sysprof-column-layer.h"
#include "sysprof-normalized-series.h"

struct _SysprofColumnLayer
{
  SysprofChartLayer        parent_instance;

  SysprofAxis             *x_axis;
  SysprofAxis             *y_axis;

  SysprofXYSeries         *series;
  SysprofNormalizedSeries *normal_x;
  SysprofNormalizedSeries *normal_y;

  GdkRGBA                  color;
  GdkRGBA                  hover_color;
};

enum {
  PROP_0,
  PROP_COLOR,
  PROP_HOVER_COLOR,
  PROP_SERIES,
  PROP_X_AXIS,
  PROP_Y_AXIS,
  N_PROPS
};

G_DEFINE_FINAL_TYPE (SysprofColumnLayer, sysprof_column_layer, SYSPROF_TYPE_CHART_LAYER)

static GParamSpec *properties [N_PROPS];

static void
sysprof_column_layer_snapshot (GtkWidget   *widget,
                               GtkSnapshot *snapshot)
{
  SysprofColumnLayer *self = (SysprofColumnLayer *)widget;
  graphene_matrix_t flip_y;
  const float *x_values;
  const float *y_values;
  guint n_x_values;
  guint n_y_values;
  guint n_values;
  int width;
  int height;

  g_assert (SYSPROF_IS_COLUMN_LAYER (self));
  g_assert (GTK_IS_SNAPSHOT (snapshot));

  width = gtk_widget_get_width (widget);
  height = gtk_widget_get_height (widget);

  if (width == 0 || height == 0 || self->color.alpha == 0)
    return;

  x_values = sysprof_normalized_series_get_values (self->normal_x, &n_x_values);
  y_values = sysprof_normalized_series_get_values (self->normal_y, &n_y_values);
  n_values = MIN (n_x_values, n_y_values);

  if (x_values == NULL || y_values == NULL)
    return;

  gtk_snapshot_save (snapshot);

  graphene_matrix_init_from_2d (&flip_y, 1, 0, 0, -1, 0, height);
  gtk_snapshot_transform_matrix (snapshot, &flip_y);

  for (guint i = 0; i < n_values; i++)
    {
      int line_height = ceilf (y_values[i] * height);

      gtk_snapshot_append_color (snapshot,
                                 &self->color,
                                 &GRAPHENE_RECT_INIT (x_values[i] * width,
                                                      0,
                                                      1,
                                                      line_height));
    }

  gtk_snapshot_restore (snapshot);
}

#if 0
static const SysprofXYSeriesValue *
sysprof_column_layer_get_value_at_coord (SysprofColumnLayer *self,
                                         double              x,
                                         double              y,
                                         graphene_rect_t    *area)
{
  const SysprofXYSeriesValue *values;
  graphene_point_t point;
  double min_x, max_x;
  double min_y, max_y;
  guint line_width;
  guint n_values;
  int width;
  int height;

  g_assert (SYSPROF_IS_COLUMN_LAYER (self));

  width = gtk_widget_get_width (GTK_WIDGET (self));
  height = gtk_widget_get_height (GTK_WIDGET (self));

  if (width == 0 || height == 0)
    return NULL;

  if (self->series == NULL ||
      !(values = sysprof_xy_series_get_values (self->series, &n_values)))
    return NULL;

  point = GRAPHENE_POINT_INIT (x, y);

  sysprof_xy_series_get_range (self->series, &min_x, &min_y, &max_x, &max_y);

  line_width = MAX (1, width / (max_x - min_x));

  for (guint i = 0; i < n_values; i++)
    {
      const SysprofXYSeriesValue *v = &values[i];
      int line_height = ceilf (v->y * height);
      graphene_rect_t rect = GRAPHENE_RECT_INIT (v->x * width,
                                                 height - line_height,
                                                 line_width,
                                                 line_height);

      if (graphene_rect_contains_point (&rect, &point))
        {
          if (area != NULL)
            *area = rect;
          return v;
        }
    }

  return NULL;
}
#endif

static void
sysprof_column_layer_snapshot_motion (SysprofChartLayer *layer,
                                      GtkSnapshot       *snapshot,
                                      double             x,
                                      double             y)
{
#if 0
  SysprofColumnLayer *self = (SysprofColumnLayer *)layer;
  const SysprofXYSeriesValue *v;
  graphene_rect_t rect;

  g_assert (SYSPROF_IS_COLUMN_LAYER (self));
  g_assert (GTK_IS_SNAPSHOT (snapshot));

  if ((v = sysprof_column_layer_get_value_at_coord (self, x, y, &rect)))
    gtk_snapshot_append_color (snapshot, &self->hover_color, &rect);
#endif
}

static gpointer
sysprof_column_layer_lookup_item (SysprofChartLayer *layer,
                                  double             x,
                                  double             y)
{
#if 0
  SysprofColumnLayer *self = (SysprofColumnLayer *)layer;
  const SysprofXYSeriesValue *v;

  g_assert (SYSPROF_IS_COLUMN_LAYER (self));

  if ((v = sysprof_column_layer_get_value_at_coord (self, x, y, NULL)))
    return g_list_model_get_item (sysprof_xy_series_get_model (self->series), v->index);
#endif

  return NULL;
}

static void
sysprof_column_layer_dispose (GObject *object)
{
  SysprofColumnLayer *self = (SysprofColumnLayer *)object;

  g_clear_object (&self->series);

  G_OBJECT_CLASS (sysprof_column_layer_parent_class)->dispose (object);
}

static void
sysprof_column_layer_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  SysprofColumnLayer *self = SYSPROF_COLUMN_LAYER (object);

  switch (prop_id)
    {
    case PROP_COLOR:
      g_value_set_boxed (value, &self->color);
      break;

    case PROP_HOVER_COLOR:
      g_value_set_boxed (value, &self->hover_color);
      break;

    case PROP_SERIES:
      g_value_set_object (value, self->series);
      break;

    case PROP_X_AXIS:
      g_value_set_object (value, self->x_axis);
      break;

    case PROP_Y_AXIS:
      g_value_set_object (value, self->y_axis);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
sysprof_column_layer_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  SysprofColumnLayer *self = SYSPROF_COLUMN_LAYER (object);

  switch (prop_id)
    {
    case PROP_COLOR:
      sysprof_column_layer_set_color (self, g_value_get_boxed (value));
      break;

    case PROP_HOVER_COLOR:
      sysprof_column_layer_set_hover_color (self, g_value_get_boxed (value));
      break;

    case PROP_SERIES:
      sysprof_column_layer_set_series (self, g_value_get_object (value));
      break;

    case PROP_X_AXIS:
      sysprof_column_layer_set_x_axis (self, g_value_get_object (value));
      break;

    case PROP_Y_AXIS:
      sysprof_column_layer_set_y_axis (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
sysprof_column_layer_class_init (SysprofColumnLayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  SysprofChartLayerClass *chart_layer_class = SYSPROF_CHART_LAYER_CLASS (klass);

  object_class->dispose = sysprof_column_layer_dispose;
  object_class->get_property = sysprof_column_layer_get_property;
  object_class->set_property = sysprof_column_layer_set_property;

  widget_class->snapshot = sysprof_column_layer_snapshot;

  chart_layer_class->lookup_item = sysprof_column_layer_lookup_item;
  chart_layer_class->snapshot_motion = sysprof_column_layer_snapshot_motion;

  properties[PROP_COLOR] =
    g_param_spec_boxed ("color", NULL, NULL,
                        GDK_TYPE_RGBA,
                        (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties[PROP_HOVER_COLOR] =
    g_param_spec_boxed ("hover-color", NULL, NULL,
                        GDK_TYPE_RGBA,
                        (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties[PROP_SERIES] =
    g_param_spec_object ("series", NULL, NULL,
                         SYSPROF_TYPE_XY_SERIES,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties[PROP_X_AXIS] =
    g_param_spec_object ("x-axis", NULL, NULL,
                         SYSPROF_TYPE_AXIS,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties[PROP_Y_AXIS] =
    g_param_spec_object ("y-axis", NULL, NULL,
                         SYSPROF_TYPE_AXIS,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
sysprof_column_layer_init (SysprofColumnLayer *self)
{
  gdk_rgba_parse (&self->color, "#000");
  gdk_rgba_parse (&self->hover_color, "#F00");

  self->normal_x = g_object_new (SYSPROF_TYPE_NORMALIZED_SERIES, NULL);
  self->normal_y = g_object_new (SYSPROF_TYPE_NORMALIZED_SERIES, NULL);
}

SysprofChartLayer *
sysprof_column_layer_new (void)
{
  return g_object_new (SYSPROF_TYPE_COLUMN_LAYER, NULL);
}

const GdkRGBA *
sysprof_column_layer_get_color (SysprofColumnLayer *self)
{
  g_return_val_if_fail (SYSPROF_IS_COLUMN_LAYER (self), NULL);

  return &self->color;
}

void
sysprof_column_layer_set_color (SysprofColumnLayer *self,
                                const GdkRGBA      *color)
{
  static const GdkRGBA black = {0,0,0,1};

  g_return_if_fail (SYSPROF_IS_COLUMN_LAYER (self));

  if (color == NULL)
    color = &black;

  if (!gdk_rgba_equal (&self->color, color))
    {
      self->color = *color;

      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_COLOR]);

      gtk_widget_queue_draw (GTK_WIDGET (self));
    }
}

const GdkRGBA *
sysprof_column_layer_get_hover_color (SysprofColumnLayer *self)
{
  g_return_val_if_fail (SYSPROF_IS_COLUMN_LAYER (self), NULL);

  return &self->hover_color;
}

void
sysprof_column_layer_set_hover_color (SysprofColumnLayer *self,
                                      const GdkRGBA      *hover_color)
{
  static const GdkRGBA red = {1,0,0,1};

  g_return_if_fail (SYSPROF_IS_COLUMN_LAYER (self));

  if (hover_color == NULL)
    hover_color = &red;

  if (!gdk_rgba_equal (&self->hover_color, hover_color))
    {
      self->hover_color = *hover_color;

      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_HOVER_COLOR]);

      gtk_widget_queue_draw (GTK_WIDGET (self));
    }
}

/**
 * sysprof_column_layer_get_series:
 * @self: a #SysprofColumnLayer
 *
 * Gets the data series to be drawn.
 *
 * Returns: (transfer none) (nullable): a #SysprofXYSeries or %NULL
 */
SysprofXYSeries *
sysprof_column_layer_get_series (SysprofColumnLayer *self)
{
  g_return_val_if_fail (SYSPROF_IS_COLUMN_LAYER (self), NULL);

  return self->series;
}

void
sysprof_column_layer_set_series (SysprofColumnLayer *self,
                                 SysprofXYSeries    *series)
{
  g_return_if_fail (SYSPROF_IS_COLUMN_LAYER (self));

  if (g_set_object (&self->series, series))
    {
      sysprof_normalized_series_set_series (self->normal_x, SYSPROF_SERIES (series));
      sysprof_normalized_series_set_series (self->normal_y, SYSPROF_SERIES (series));

      if (series)
        {
          g_object_bind_property (series, "x-expression",
                                  self->normal_x, "expression",
                                  G_BINDING_SYNC_CREATE);
          g_object_bind_property (series, "y-expression",
                                  self->normal_y, "expression",
                                  G_BINDING_SYNC_CREATE);
        }

      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SERIES]);

      gtk_widget_queue_draw (GTK_WIDGET (self));
    }
}

/**
 * sysprof_column_layer_get_x_axis:
 * @self: a #SysprofColumnLayer
 *
 * Gets the axis represeting X.
 *
 * Returns: (transfer none) (nullable): the X axis
 */
SysprofAxis *
sysprof_column_layer_get_x_axis (SysprofColumnLayer *self)
{
  g_return_val_if_fail (SYSPROF_IS_COLUMN_LAYER (self), NULL);

  return self->x_axis;
}

void
sysprof_column_layer_set_x_axis (SysprofColumnLayer *self,
                                 SysprofAxis        *x_axis)
{
  g_return_if_fail (SYSPROF_IS_COLUMN_LAYER (self));
  g_return_if_fail (!x_axis || SYSPROF_IS_AXIS (x_axis));

  if (g_set_object (&self->x_axis, x_axis))
    {
      sysprof_normalized_series_set_axis (self->normal_x, x_axis);

      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_X_AXIS]);

      gtk_widget_queue_draw (GTK_WIDGET (self));
    }
}

/**
 * sysprof_column_layer_get_y_axis:
 * @self: a #SysprofColumnLayer
 *
 * Gets the axis represeting Y.
 *
 * Returns: (transfer none) (nullable): the Y axis
 */
SysprofAxis *
sysprof_column_layer_get_y_axis (SysprofColumnLayer *self)
{
  g_return_val_if_fail (SYSPROF_IS_COLUMN_LAYER (self), NULL);

  return self->y_axis;
}

void
sysprof_column_layer_set_y_axis (SysprofColumnLayer *self,
                                 SysprofAxis        *y_axis)
{
  g_return_if_fail (SYSPROF_IS_COLUMN_LAYER (self));
  g_return_if_fail (!y_axis || SYSPROF_IS_AXIS (y_axis));

  if (g_set_object (&self->y_axis, y_axis))
    {
      sysprof_normalized_series_set_axis (self->normal_y, y_axis);

      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_Y_AXIS]);

      gtk_widget_queue_draw (GTK_WIDGET (self));
    }
}

