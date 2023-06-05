/* sysprof-sampler.c
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

#include "sysprof-instrument-private.h"
#include "sysprof-perf-event-stream-private.h"
#include "sysprof-recording-private.h"
#include "sysprof-sampler.h"

#define N_WAKEUP_EVENTS 149

struct _SysprofSampler
{
  SysprofInstrument parent_instance;
  GPtrArray *perf_event_streams;
};

struct _SysprofSamplerClass
{
  SysprofInstrumentClass parent_class;
};

G_DEFINE_FINAL_TYPE (SysprofSampler, sysprof_sampler, SYSPROF_TYPE_INSTRUMENT)

static char **
sysprof_sampler_list_required_policy (SysprofInstrument *instrument)
{
  static const char *policy[] = {"org.gnome.sysprof3.profile", NULL};

  return g_strdupv ((char **)policy);
}

static inline void
realign (gsize *pos,
         gsize  align)
{
  *pos = (*pos + align - 1) & ~(align - 1);
}

static void
sysprof_sampler_add_callback (SysprofCaptureWriter            *writer,
                              int                              cpu,
                              const SysprofPerfEventCallchain *sample)
{
  const guint64 *ips;
  guint64 trace[3];
  int n_ips;

  g_assert (writer != NULL);
  g_assert (sample != NULL);

  ips = sample->ips;
  n_ips = sample->n_ips;

  if (n_ips == 0)
    {
      if (sample->header.misc & PERF_RECORD_MISC_KERNEL)
        {
          trace[0] = PERF_CONTEXT_KERNEL;
          trace[1] = sample->ip;
          trace[2] = PERF_CONTEXT_USER;

          ips = trace;
          n_ips = 3;
        }
      else
        {
          trace[0] = PERF_CONTEXT_USER;
          trace[1] = sample->ip;

          ips = trace;
          n_ips = 2;
        }
    }

  sysprof_capture_writer_add_sample (writer,
                                     sample->time,
                                     cpu,
                                     sample->pid,
                                     sample->tid,
                                     ips,
                                     n_ips);
}

static void
sysprof_sampler_perf_event_stream_cb (const SysprofPerfEvent *event,
                                      guint                   cpu,
                                      gpointer                user_data)
{
  SysprofCaptureWriter *writer = user_data;
  gsize offset;
  gint64 time;

  g_assert (writer != NULL);
  g_assert (event != NULL);

  switch (event->header.type)
    {
    case PERF_RECORD_COMM:
      offset = strlen (event->comm.comm) + 1;
      realign (&offset, sizeof (guint64));
      offset += sizeof (GPid) + sizeof (GPid);
      memcpy (&time, event->comm.comm + offset, sizeof time);

      if (event->comm.pid == event->comm.tid)
        sysprof_capture_writer_add_process (writer,
                                            time,
                                            cpu,
                                            event->comm.pid,
                                            event->comm.comm);

      break;

    case PERF_RECORD_EXIT:
      /* Ignore fork exits for now */
      if (event->exit.tid != event->exit.pid)
        break;

      sysprof_capture_writer_add_exit (writer,
                                       event->exit.time,
                                       cpu,
                                       event->exit.pid);

      break;

    case PERF_RECORD_FORK:
      sysprof_capture_writer_add_fork (writer,
                                       event->fork.time,
                                       cpu,
                                       event->fork.ptid,
                                       event->fork.tid);

      /*
       * TODO: We should add support for "follow fork" of the GPid if we are
       *       targetting it.
       */

      break;

    case PERF_RECORD_LOST:
      break;

    case PERF_RECORD_MMAP:
      offset = strlen (event->mmap.filename) + 1;
      realign (&offset, sizeof (guint64));
      offset += sizeof (GPid) + sizeof (GPid);
      memcpy (&time, event->mmap.filename + offset, sizeof time);

      sysprof_capture_writer_add_map (writer,
                                      time,
                                      cpu,
                                      event->mmap.pid,
                                      event->mmap.addr,
                                      event->mmap.addr + event->mmap.len,
                                      event->mmap.pgoff,
                                      0,
                                      event->mmap.filename);

      break;

    case PERF_RECORD_READ:
      break;

    case PERF_RECORD_SAMPLE:
      sysprof_sampler_add_callback (writer, cpu, &event->callchain);
      break;

    case PERF_RECORD_THROTTLE:
    case PERF_RECORD_UNTHROTTLE:
    default:
      break;
    }
}

typedef struct _Prepare
{
  SysprofRecording *recording;
  SysprofSampler *sampler;
} Prepare;

static void
prepare_free (Prepare *prepare)
{
  g_clear_object (&prepare->recording);
  g_clear_object (&prepare->sampler);
  g_free (prepare);
}

static DexFuture *
sysprof_sampler_prepare_fiber (gpointer user_data)
{
  Prepare *prepare = user_data;
  SysprofCaptureWriter *writer;
  g_autoptr(GDBusConnection) connection = NULL;
  g_autoptr(GPtrArray) futures = NULL;
  g_autoptr(GError) error = NULL;
  struct perf_event_attr attr = {0};
  guint n_cpu;

  g_assert (prepare != NULL);
  g_assert (SYSPROF_IS_RECORDING (prepare->recording));
  g_assert (SYSPROF_IS_SAMPLER (prepare->sampler));

  /* First thing we need to do is to ensure the consumer has
   * access to kallsyms, which may be from a machine, or boot
   * different than this boot (and therefore symbols exist in
   * different locations). Embed the kallsyms, but gzip it as
   * those files can be quite large.
   */
  dex_await (_sysprof_recording_add_file (recording, "/proc/kallsyms", TRUE), NULL);

  /* Now create a SysprofPerfEventStream for every CPU on the
   * system. Linux Perf will only let us create a stream for
   * a single PID on all CPU, or all PID on a single CPU. So
   * we create one per-CPU and stream those results into the
   * capture file during recording.
   *
   * Previously, we supported recording a single process but
   * that is more effort than it is worth, since virtually
   * nobody uses Sysprof that way.
   */
  n_cpu = g_get_num_processors ();
  futures = g_ptr_array_new_with_free_func (dex_unref);
  writer = _sysprof_recording_writer (prepare->recording);

  attr.sample_type = PERF_SAMPLE_IP
                   | PERF_SAMPLE_TID
                   | PERF_SAMPLE_IDENTIFIER
                   | PERF_SAMPLE_CALLCHAIN
                   | PERF_SAMPLE_TIME;
  attr.wakeup_events = N_WAKEUP_EVENTS;
  attr.disabled = TRUE;
  attr.mmap = 1;
  attr.comm = 1;
  attr.task = 1;
  attr.exclude_idle = 1;
  attr.sample_id_all = 1;

#ifdef HAVE_PERF_CLOCKID
  attr.clockid = sysprof_clock;
  attr.use_clockid = 1;
#endif

  attr.size = sizeof attr;

  attr.type = PERF_TYPE_HARDWARE;
  attr.config = PERF_COUNT_HW_CPU_CYCLES;
  attr.sample_period = 1200000;

  if (!(connection = dex_await_object (dex_bus_get (G_BUS_TYPE_SYSTEM), &error)))
    return dex_future_new_for_error (g_steal_pointer (&error));

  /* Pipeline our request for n_cpu perf_event_open calls and then
   * await them all to complete.
   */
  for (guint i = 0; i < n_cpu; i++)
    g_ptr_array_add (futures,
                     sysprof_perf_event_stream_new (connection,
                                                    &attr,
                                                    i,
                                                    -1,
                                                    0,
                                                    sysprof_sampler_perf_event_stream_cb,
                                                    sysprof_capture_writer_ref (writer),
                                                    (GDestroyNotify)sysprof_capture_writer_unref));

  if (!dex_await (dex_future_allv ((DexFuture **)futures->pdata, futures->len), &error))
    return dex_future_new_for_error (g_steal_pointer (&error));

  /* Save each of the streams (currently corked), so that we can
   * uncork them while recording. We already checked that all the
   * futures have succeeded above, so dex_await_object() must
   * always return an object for each sub-future.
   */
  for (guint i = 0; i < futures->len; i++)
    {
      DexFuture *future = g_ptr_array_index (futures, i);
      g_autoptr(SysprofPerfEventStream) stream = NULL;
      g_autoptr(GError) stream_error = NULL;

      if ((stream = dex_await_object (dex_ref (future), &stream_error)))
        g_ptr_array_add (prepare->sampler->perf_event_streams, g_steal_pointer (&stream));
    }

  return dex_future_new_for_boolean (TRUE);
}

static DexFuture *
sysprof_sampler_prepare (SysprofInstrument *instrument,
                         SysprofRecording  *recording)
{
  SysprofSampler *self = (SysprofSampler *)instrument;
  Prepare *prepare;

  g_assert (SYSPROF_IS_INSTRUMENT (instrument));
  g_assert (SYSPROF_IS_RECORDING (recording));

  prepare = g_new0 (Prepare, 1);
  prepare->recording = g_object_ref (recording);
  prepare->sampler = g_object_ref (self);

  return dex_scheduler_spawn (NULL, 0,
                              sysprof_sampler_prepare_fiber,
                              prepare,
                              (GDestroyNotify)prepare_free);
}

typedef struct _Record
{
  SysprofRecording *recording;
  SysprofSampler *sampler;
  DexFuture *cancellable;
} Record;

static void
record_free (Record *record)
{
  g_clear_object (&record->recording);
  g_clear_object (&record->sampler);
  dex_clear (&record->cancellable);
  g_free (record);
}

static DexFuture *
sysprof_sampler_record_fiber (gpointer user_data)
{
  Record *record = user_data;

  g_assert (record != NULL);
  g_assert (SYSPROF_IS_SAMPLER (record->sampler));
  g_assert (SYSPROF_IS_RECORDING (record->recording));
  g_assert (DEX_IS_FUTURE (record->cancellable));

  for (guint i = 0; i < record->sampler->perf_event_streams->len; i++)
    {
      SysprofPerfEventStream *stream = g_ptr_array_index (record->sampler->perf_event_streams, i);
      g_autoptr(GError) error = NULL;

      if (!sysprof_perf_event_stream_enable (stream, &error))
        g_debug ("%s", error->message);
      g_debug ("Sampler %d enabled", i);
    }

  dex_await (dex_ref (record->cancellable), NULL);

  for (guint i = 0; i < record->sampler->perf_event_streams->len; i++)
    {
      SysprofPerfEventStream *stream = g_ptr_array_index (record->sampler->perf_event_streams, i);
      g_autoptr(GError) error = NULL;

      if (!sysprof_perf_event_stream_disable (stream, &error))
        g_debug ("%s", error->message);
      else
        g_debug ("Sampler %d disabled", i);
    }

  return dex_future_new_for_boolean (TRUE);
}

static DexFuture *
sysprof_sampler_record (SysprofInstrument *instrument,
                        SysprofRecording  *recording,
                        GCancellable      *cancellable)
{
  SysprofSampler *self = (SysprofSampler *)instrument;
  Record *record;

  g_assert (SYSPROF_IS_INSTRUMENT (instrument));
  g_assert (SYSPROF_IS_RECORDING (recording));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  record = g_new0 (Record, 1);
  record->recording = g_object_ref (recording);
  record->sampler = g_object_ref (self);
  record->cancellable = dex_cancellable_new_from_cancellable (cancellable);

  return dex_scheduler_spawn (NULL, 0,
                              sysprof_sampler_record_fiber,
                              record,
                              (GDestroyNotify)record_free);
}

static void
sysprof_sampler_finalize (GObject *object)
{
  SysprofSampler *self = (SysprofSampler *)object;

  g_clear_pointer (&self->perf_event_streams, g_ptr_array_unref);

  G_OBJECT_CLASS (sysprof_sampler_parent_class)->finalize (object);
}

static void
sysprof_sampler_class_init (SysprofSamplerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SysprofInstrumentClass *instrument_class = SYSPROF_INSTRUMENT_CLASS (klass);

  object_class->finalize = sysprof_sampler_finalize;

  instrument_class->list_required_policy = sysprof_sampler_list_required_policy;
  instrument_class->prepare = sysprof_sampler_prepare;
  instrument_class->record = sysprof_sampler_record;
}

static void
sysprof_sampler_init (SysprofSampler *self)
{
  self->perf_event_streams = g_ptr_array_new_with_free_func (g_object_unref);
}

SysprofInstrument *
sysprof_sampler_new (void)
{
  return g_object_new (SYSPROF_TYPE_SAMPLER, NULL);
}
