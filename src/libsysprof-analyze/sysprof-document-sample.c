/* sysprof-document-sample.c
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

#include "sysprof-document-frame-private.h"
#include "sysprof-document-sample.h"

struct _SysprofDocumentSample
{
  SysprofDocumentFrame parent_instance;
};

struct _SysprofDocumentSampleClass
{
  SysprofDocumentFrameClass parent_class;
};

enum {
  PROP_0,
  PROP_DEPTH,
  PROP_TID,
  N_PROPS
};

G_DEFINE_FINAL_TYPE (SysprofDocumentSample, sysprof_document_sample, SYSPROF_TYPE_DOCUMENT_FRAME)

static GParamSpec *properties [N_PROPS];

static void
sysprof_document_sample_get_property (GObject    *object,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  SysprofDocumentSample *self = SYSPROF_DOCUMENT_SAMPLE (object);

  switch (prop_id)
    {
    case PROP_DEPTH:
      g_value_set_uint (value, sysprof_document_sample_get_depth (self));
      break;

    case PROP_TID:
      g_value_set_int (value, sysprof_document_sample_get_tid (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
sysprof_document_sample_class_init (SysprofDocumentSampleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = sysprof_document_sample_get_property;

  /**
   * SysprofDocumentSample:tid:
   *
   * The task-id or thread-id of the thread which was sampled.
   *
   * On Linux, this is generally set to the value of the gettid() syscall.
   *
   * Since: 45
   */
  properties [PROP_TID] =
    g_param_spec_int ("tid", NULL, NULL,
                      G_MININT32, G_MAXINT32, 0,
                      (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  /**
   * SysprofDocumentSample:depth:
   *
   * The depth of the stack trace.
   *
   * Since: 45
   */
  properties [PROP_DEPTH] =
    g_param_spec_uint ("depth", NULL, NULL,
                       0, G_MAXUINT32, 0,
                       (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
sysprof_document_sample_init (SysprofDocumentSample *self)
{
}

guint
sysprof_document_sample_get_depth (SysprofDocumentSample *self)
{
  const SysprofCaptureSample *sample;

  g_return_val_if_fail (SYSPROF_IS_DOCUMENT_SAMPLE (self), 0);

  sample = SYSPROF_DOCUMENT_FRAME_GET (self, SysprofCaptureSample);

  return SYSPROF_DOCUMENT_FRAME_UINT32 (self, sample->n_addrs);
}

int
sysprof_document_sample_get_tid (SysprofDocumentSample *self)
{
  const SysprofCaptureSample *sample;

  g_return_val_if_fail (SYSPROF_IS_DOCUMENT_SAMPLE (self), 0);

  sample = SYSPROF_DOCUMENT_FRAME_GET (self, SysprofCaptureSample);

  return SYSPROF_DOCUMENT_FRAME_INT32 (self, sample->tid);
}
