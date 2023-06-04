/* sysprof-perf-event-stream.c
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

/* Sysprof -- Sampling, systemwide CPU profiler
 * Copyright 2004, Red Hat, Inc.
 * Copyright 2004, 2005, Soeren Sandmann
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <errno.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <gio/gunixfdlist.h>
#ifdef HAVE_STDATOMIC_H
# include <stdatomic.h>
#endif
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "sysprof-perf-event-stream-private.h"

/*
 * Number of pages to map for the ring buffer.  We map one additional buffer
 * at the beginning for header information to communicate with perf.
 */
#define N_PAGES 32

struct _SysprofPerfEventStream
{
  GObject parent_instance;

  GDBusConnection *connection;

  GSource *source;

  struct perf_event_attr attr;

  int cpu;
  int group_fd;
  gulong flags;

  SysprofPerfEventCallback callback;
  gpointer callback_data;
  GDestroyNotify callback_data_destroy;

  DexPromise *promise;

  int perf_fd;
  gpointer perf_fd_tag;

  struct perf_event_mmap_page *map;
  guint8 *map_data;
  guint64 tail;

  guint active : 1;
};

typedef struct _SysprofPerfEventSource
{
  GSource source;
  SysprofPerfEventStream *stream;
} SysprofPerfEventSource;

enum {
  PROP_0,
  PROP_ACTIVE,
  N_PROPS
};

G_DEFINE_FINAL_TYPE (SysprofPerfEventStream, sysprof_perf_event_stream, G_TYPE_OBJECT)

static GParamSpec *properties [N_PROPS];

static void
sysprof_perf_event_stream_flush (SysprofPerfEventStream *self)
{
  guint64 n_bytes = N_PAGES * sysprof_getpagesize ();
  guint64 mask = n_bytes - 1;
  guint64 head;
  guint64 tail;

  g_assert (SYSPROF_IS_PERF_EVENT_STREAM (self));

  tail = self->tail;
  head = self->map->data_head;

#ifdef HAVE_STDATOMIC_H
  atomic_thread_fence (memory_order_acquire);
#elif G_GNUC_CHECK_VERSION(3, 0)
  __sync_synchronize ();
#endif

  if (head < tail)
    tail = head;

  while ((head - tail) >= sizeof (struct perf_event_header))
    {
      g_autofree guint8 *free_me = NULL;
      struct perf_event_header *header;
      guint8 buffer[4096];

      /* Note that:
       *
       * - perf events are a multiple of 64 bits
       * - the perf event header is 64 bits
       * - the data area is a multiple of 64 bits
       *
       * which means there will always be space for one header, which means we
       * can safely dereference the size field.
       */
      header = (struct perf_event_header *)(gpointer)(self->map_data + (tail & mask));

      if (header->size > head - tail)
        {
          /* The kernel did not generate a complete event.
           * I don't think that can happen, but we may as well
           * be paranoid.
           */
          break;
        }

      if (self->map_data + (tail & mask) + header->size > self->map_data + n_bytes)
        {
          gint n_before;
          gint n_after;
          guint8 *b;

          if (header->size > sizeof buffer)
            free_me = b = g_malloc (header->size);
          else
            b = buffer;

          n_after = (tail & mask) + header->size - n_bytes;
          n_before = header->size - n_after;

          memcpy (b, self->map_data + (tail & mask), n_before);
          memcpy (b + n_before, self->map_data, n_after);

          header = (struct perf_event_header *)(gpointer)b;
        }

      if (self->callback != NULL)
        self->callback ((SysprofPerfEvent *)header, self->cpu, self->callback_data);

      tail += header->size;
    }

  self->tail = tail;

#ifdef HAVE_STDATOMIC_H
  atomic_thread_fence (memory_order_seq_cst);
#elif G_GNUC_CHECK_VERSION(3, 0)
  __sync_synchronize ();
#endif

  self->map->data_tail = tail;
}

static gboolean
sysprof_perf_event_source_dispatch (GSource     *source,
                                    GSourceFunc  callback,
                                    gpointer     user_data)
{
  return callback ? callback (user_data) : G_SOURCE_CONTINUE;
}

static GSourceFuncs source_funcs = {
  .dispatch = sysprof_perf_event_source_dispatch,
};

static gboolean
sysprof_perf_event_stream_dispatch (gpointer user_data)
{
  SysprofPerfEventStream *self = user_data;

  g_assert (SYSPROF_IS_PERF_EVENT_STREAM (self));

  sysprof_perf_event_stream_flush (self);

  return G_SOURCE_REMOVE;
}

static void
sysprof_perf_event_stream_finalize (GObject *object)
{
  SysprofPerfEventStream *self = (SysprofPerfEventStream *)object;

  if (self->callback_data_destroy)
    {
      self->callback_data_destroy (self->callback_data);
      self->callback_data_destroy = NULL;
      self->callback_data = NULL;
    }

  self->callback = NULL;

  dex_clear (&self->promise);

  g_clear_object (&self->connection);

  if (self->source != NULL)
    {
      g_source_destroy (self->source);
      g_source_unref (self->source);
      self->source = NULL;
    }

  g_clear_fd (&self->perf_fd, NULL);

  G_OBJECT_CLASS (sysprof_perf_event_stream_parent_class)->finalize (object);
}

static void
sysprof_perf_event_stream_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  SysprofPerfEventStream *self = SYSPROF_PERF_EVENT_STREAM (object);

  switch (prop_id)
    {
    case PROP_ACTIVE:
      g_value_set_boolean (value, self->active);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
sysprof_perf_event_stream_class_init (SysprofPerfEventStreamClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = sysprof_perf_event_stream_finalize;
  object_class->get_property = sysprof_perf_event_stream_get_property;

  properties[PROP_ACTIVE] =
    g_param_spec_boolean ("active", NULL, NULL,
                          FALSE,
                          (G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
sysprof_perf_event_stream_init (SysprofPerfEventStream *self)
{
  self->perf_fd = -1;
}

static void
sysprof_perf_event_stream_new_cb (GObject      *object,
                                  GAsyncResult *result,
                                  gpointer      user_data)
{
  GDBusConnection *connection = (GDBusConnection *)object;
  g_autoptr(SysprofPerfEventStream) self = user_data;
  g_autoptr(GUnixFDList) fd_list = NULL;
  g_autoptr(GVariant) ret = NULL;
  g_autoptr(GError) error = NULL;

  g_assert (G_IS_DBUS_CONNECTION (connection));
  g_assert (G_IS_ASYNC_RESULT (result));
  g_assert (SYSPROF_IS_PERF_EVENT_STREAM (self));

  if ((ret = g_dbus_connection_call_with_unix_fd_list_finish (connection, &fd_list, result, &error)))
    {
      int handle;
      int fd;

      g_variant_get (ret, "(h)", &handle);

      if (-1 != (fd = g_unix_fd_list_get (fd_list, handle, &error)))
        self->perf_fd = fd;
    }

  if (error != NULL)
    dex_promise_reject (self->promise, g_steal_pointer (&error));
  else
    dex_promise_resolve_object (self->promise, g_object_ref (self));

  dex_clear (&self->promise);
}

DexFuture *
sysprof_perf_event_stream_new (GDBusConnection          *connection,
                               struct perf_event_attr   *attr,
                               int                       cpu,
                               int                       group_fd,
                               gulong                    flags,
                               SysprofPerfEventCallback  callback,
                               gpointer                  callback_data,
                               GDestroyNotify            callback_data_destroy)
{
  g_autoptr(SysprofPerfEventStream) self = NULL;
  g_autoptr(GUnixFDList) fd_list = NULL;
  g_autoptr(DexPromise) promise = NULL;
  GVariantDict options = G_VARIANT_DICT_INIT (NULL);
  int group_fd_handle = -1;

  g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);
  g_return_val_if_fail (attr != NULL, NULL);
  g_return_val_if_fail (cpu > -1, NULL);
  g_return_val_if_fail (group_fd >= -1, NULL);

  promise = dex_promise_new ();

  self = g_object_new (SYSPROF_TYPE_PERF_EVENT_STREAM, NULL);
  self->connection = g_object_ref (connection);
  self->attr = *attr;
  self->cpu = cpu;
  self->group_fd = group_fd;
  self->flags = flags;
  self->callback = callback;
  self->callback_data = callback_data;
  self->callback_data_destroy = callback_data_destroy;
  self->promise = dex_ref (promise);
  self->source = g_source_new (&source_funcs, sizeof (SysprofPerfEventSource));

  g_source_set_callback (self->source, sysprof_perf_event_stream_dispatch, self, NULL);
  g_source_set_name (self->source, "[perf]");
  g_source_attach (self->source, NULL);

  fd_list = g_unix_fd_list_new ();

  if (group_fd > -1)
    group_fd_handle = g_unix_fd_list_append (fd_list, group_fd, NULL);

  g_dbus_connection_call_with_unix_fd_list (connection,
                                            "org.gnome.Sysprof3",
                                            "/org/gnome/Sysprof3/Service",
                                            "org.gnome.Sysprof3.Service",
                                            "PerfEventOpen",
                                            g_variant_new ("(@a{sv}ii@ht)",
                                                           g_variant_dict_end (&options),
                                                           -1,
                                                           cpu,
                                                           group_fd_handle,
                                                           flags),
                                            G_VARIANT_TYPE ("(h)"),
                                            G_DBUS_CALL_FLAGS_NONE,
                                            G_MAXUINT,
                                            fd_list,
                                            dex_promise_get_cancellable (promise),
                                            sysprof_perf_event_stream_new_cb,
                                            g_object_ref (self));

  return DEX_FUTURE (g_steal_pointer (&promise));
}

gboolean
sysprof_perf_event_stream_enable (SysprofPerfEventStream  *self,
                                  GError                 **error)
{
  g_return_val_if_fail (SYSPROF_IS_PERF_EVENT_STREAM (self), FALSE);

  if (self->active)
    return TRUE;

  if (0 != ioctl (self->perf_fd, PERF_EVENT_IOC_ENABLE))
    {
      int errsv = errno;
      g_set_error_literal (error,
                           G_IO_ERROR,
                           g_io_error_from_errno (errsv),
                           g_strerror (errsv));
      return FALSE;
    }

  self->active = TRUE;

  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_ACTIVE]);

  return TRUE;
}

gboolean
sysprof_perf_event_stream_disable (SysprofPerfEventStream  *self,
                                   GError                 **error)
{
  g_return_val_if_fail (SYSPROF_IS_PERF_EVENT_STREAM (self), FALSE);

  if (!self->active)
    return TRUE;

  if (0 != ioctl (self->perf_fd, PERF_EVENT_IOC_DISABLE))
    {
      int errsv = errno;
      g_set_error_literal (error,
                           G_IO_ERROR,
                           g_io_error_from_errno (errsv),
                           g_strerror (errsv));
      return FALSE;
    }

  self->active = FALSE;

  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_ACTIVE]);

  return TRUE;
}
