/* test-capture.c
 *
 * Copyright (C) 2016 Christian Hergert <christian@hergert.me>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib/gstdio.h>

#include "sp-capture-reader.h"
#include "sp-capture-writer.h"

static void
test_reader_basic (void)
{
  SpCaptureReader *reader;
  SpCaptureWriter *writer;
  GError *error = NULL;
  gint64 t = g_get_monotonic_time ();
  guint i;
  gint r;

  writer = sp_capture_writer_new ("capture-file", 0);
  g_assert (writer != NULL);

  reader = sp_capture_reader_new ("capture-file", &error);
  g_assert_no_error (error);
  g_assert (reader != NULL);

  for (i = 0; i < 100; i++)
    {
      gchar str[16];

      g_snprintf (str, sizeof str, "%d", i);

      r = sp_capture_writer_add_map (writer, t, -1, -1, i, i + 1, i + 2, i + 3, str);
      g_assert_cmpint (r, ==, TRUE);
    }

  sp_capture_writer_flush (writer);

  for (i = 0; i < 100; i++)
    {
      SpCaptureFrameType type = -1;
      const SpCaptureMap *map;
      gchar str[16];

      g_snprintf (str, sizeof str, "%d", i);

      if (!sp_capture_reader_peek_type (reader, &type))
        g_assert_not_reached ();

      g_assert_cmpint (type, ==, SP_CAPTURE_FRAME_MAP);

      map = sp_capture_reader_read_map (reader);

      g_assert (map != NULL);
      g_assert_cmpint (map->frame.pid, ==, -1);
      g_assert_cmpint (map->frame.cpu, ==, -1);
      g_assert_cmpint (map->start, ==, i);
      g_assert_cmpint (map->end, ==, i + 1);
      g_assert_cmpint (map->offset, ==, i + 2);
      g_assert_cmpint (map->inode, ==, i + 3);
      g_assert_cmpstr (map->filename, ==, str);
    }

  for (i = 0; i < 100; i++)
    {
      r = sp_capture_writer_add_timestamp (writer, t, i, -1);
      g_assert_cmpint (r, ==, TRUE);
    }

  sp_capture_writer_flush (writer);

  for (i = 0; i < 100; i++)
    {
      SpCaptureFrameType type = -1;
      const SpCaptureTimestamp *ts;

      if (!sp_capture_reader_peek_type (reader, &type))
        g_assert_not_reached ();

      g_assert_cmpint (type, ==, SP_CAPTURE_FRAME_TIMESTAMP);

      ts = sp_capture_reader_read_timestamp (reader);

      g_assert (ts != NULL);
      g_assert_cmpint (ts->frame.cpu, ==, i);
      g_assert_cmpint (ts->frame.pid, ==, -1);
    }

  for (i = 0; i < 100; i++)
    {
      r = sp_capture_writer_add_exit (writer, t, i, -1);
      g_assert_cmpint (r, ==, TRUE);
    }

  sp_capture_writer_flush (writer);

  for (i = 0; i < 100; i++)
    {
      SpCaptureFrameType type = -1;
      const SpCaptureExit *ex;

      if (!sp_capture_reader_peek_type (reader, &type))
        g_assert_not_reached ();

      g_assert_cmpint (type, ==, SP_CAPTURE_FRAME_EXIT);

      ex = sp_capture_reader_read_exit (reader);

      g_assert (ex != NULL);
      g_assert_cmpint (ex->frame.cpu, ==, i);
      g_assert_cmpint (ex->frame.pid, ==, -1);
    }

  for (i = 0; i < 100; i++)
    {
      char cmdline[32];

      g_snprintf (cmdline, sizeof cmdline, "program-%d", i);
      r = sp_capture_writer_add_process (writer, t, -1, i, cmdline);
      g_assert_cmpint (r, ==, TRUE);
    }

  sp_capture_writer_flush (writer);

  for (i = 0; i < 100; i++)
    {
      SpCaptureFrameType type = -1;
      const SpCaptureProcess *pr;
      char str[32];

      g_snprintf (str, sizeof str, "program-%d", i);

      if (!sp_capture_reader_peek_type (reader, &type))
        g_assert_not_reached ();

      g_assert_cmpint (type, ==, SP_CAPTURE_FRAME_PROCESS);

      pr = sp_capture_reader_read_process (reader);

      g_assert (pr != NULL);
      g_assert_cmpint (pr->frame.cpu, ==, -1);
      g_assert_cmpint (pr->frame.pid, ==, i);
      g_assert_cmpstr (pr->cmdline, ==, str);
    }

  for (i = 0; i < 100; i++)
    {
      r = sp_capture_writer_add_fork (writer, t, i, -1, i);
      g_assert_cmpint (r, ==, TRUE);
    }

  sp_capture_writer_flush (writer);

  for (i = 0; i < 100; i++)
    {
      SpCaptureFrameType type = -1;
      const SpCaptureFork *ex;

      if (!sp_capture_reader_peek_type (reader, &type))
        g_assert_not_reached ();

      g_assert_cmpint (type, ==, SP_CAPTURE_FRAME_FORK);

      ex = sp_capture_reader_read_fork (reader);

      g_assert (ex != NULL);
      g_assert_cmpint (ex->frame.cpu, ==, i);
      g_assert_cmpint (ex->frame.pid, ==, -1);
      g_assert_cmpint (ex->child_pid, ==, i);
    }

  for (i = 0; i < 1000; i++)
    {
      SpCaptureAddress addr;
      gchar str[32];

      g_snprintf (str, sizeof str, "jitstring-%d", i);

      addr = sp_capture_writer_add_jitmap (writer, str);
      g_assert_cmpint (addr, ==, (i + 1) | SP_CAPTURE_JITMAP_MARK);
    }

  sp_capture_writer_flush (writer);

  i = 0;

  for (;;)
    {
      SpCaptureFrameType type = -1;
      g_autoptr(GHashTable) ret = NULL;

      if (sp_capture_reader_peek_type (reader, &type))
        g_assert_cmpint (type, ==, SP_CAPTURE_FRAME_JITMAP);
      else
        break;

      ret = sp_capture_reader_read_jitmap (reader);
      g_assert (ret != NULL);

      i += g_hash_table_size (ret);
    }

  g_assert_cmpint (1000, ==, i);

  {
    SpCaptureFrameType type = -1;

    if (sp_capture_reader_peek_type (reader, &type))
      g_assert_not_reached ();
  }

  for (i = 1; i <= 1000; i++)
    {
      SpCaptureAddress *addrs;
      guint j;

      addrs = alloca (i * sizeof *addrs);

      for (j = 0; j < i; j++)
        addrs[j] = i;

      if (!sp_capture_writer_add_sample (writer, t, -1, -1, addrs, i))
        g_assert_not_reached ();
    }

  sp_capture_writer_flush (writer);

  for (i = 1; i <= 1000; i++)
    {
      SpCaptureFrameType type = -1;
      const SpCaptureSample *sample;
      guint j;

      if (!sp_capture_reader_peek_type (reader, &type))
        g_assert_not_reached ();

      g_assert_cmpint (type, ==, SP_CAPTURE_FRAME_SAMPLE);

      sample = sp_capture_reader_read_sample (reader);

      g_assert (sample != NULL);
      g_assert_cmpint (sample->frame.time, ==, t);
      g_assert_cmpint (sample->frame.cpu, ==, -1);
      g_assert_cmpint (sample->frame.pid, ==, -1);
      g_assert_cmpint (sample->n_addrs, ==, i);

      for (j = 0; j < i; j++)
        g_assert_cmpint (sample->addrs[j], ==, i);
    }

  {
    SpCaptureFrameType type = -1;

    if (sp_capture_reader_peek_type (reader, &type))
      g_assert_not_reached ();
  }

  r = sp_capture_writer_save_as (writer, "capture-file.bak", &error);
  g_assert_no_error (error);
  g_assert (r);
  g_assert (g_file_test ("capture-file.bak", G_FILE_TEST_IS_REGULAR));

  g_clear_pointer (&writer, sp_capture_writer_unref);
  g_clear_pointer (&reader, sp_capture_reader_unref);

  reader = sp_capture_reader_new ("capture-file.bak", &error);
  g_assert_no_error (error);
  g_assert (reader != NULL);

  for (i = 0; i < 2; i++)
    {
      SpCaptureFrameType type = -1;
      guint count = 0;

      while (sp_capture_reader_peek_type (reader, &type))
        {
          count++;
          if (!sp_capture_reader_skip (reader))
            g_assert_not_reached ();
        }

      g_assert_cmpint (count, >, 1500);

      sp_capture_reader_reset (reader);
    }

  sp_capture_reader_unref (reader);

  g_unlink ("capture-file");
  g_unlink ("capture-file.bak");
}

static void
test_writer_splice (void)
{
  SpCaptureWriter *writer1;
  SpCaptureWriter *writer2;
  SpCaptureReader *reader;
  SpCaptureFrameType type;
  GError *error = NULL;
  guint i;
  gint r;

  writer1 = sp_capture_writer_new ("writer1.syscap", 0);
  writer2 = sp_capture_writer_new ("writer2.syscap", 0);

  for (i = 0; i < 1000; i++)
    sp_capture_writer_add_timestamp (writer1, SP_CAPTURE_CURRENT_TIME, -1, -1);

  r = sp_capture_writer_splice (writer1, writer2, &error);
  g_assert_no_error (error);
  g_assert (r == TRUE);

  g_clear_pointer (&writer1, sp_capture_writer_unref);
  g_clear_pointer (&writer2, sp_capture_writer_unref);

  reader = sp_capture_reader_new ("writer2.syscap", &error);
  g_assert_no_error (error);
  g_assert (reader != NULL);

  for (i = 0; i < 1000; i++)
    {
      const SpCaptureTimestamp *ts = sp_capture_reader_read_timestamp (reader);

      g_assert (ts != NULL);
      g_assert_cmpint (ts->frame.cpu, ==, -1);
      g_assert_cmpint (ts->frame.pid, ==, -1);
      g_assert_cmpint (ts->frame.time, >, 0);
    }

  r = sp_capture_reader_peek_type (reader, &type);
  g_assert_cmpint (r, ==, FALSE);

  g_clear_pointer (&reader, sp_capture_reader_unref);

  g_unlink ("writer1.syscap");
  g_unlink ("writer2.syscap");
}

static void
test_reader_splice (void)
{
  SpCaptureWriter *writer1;
  SpCaptureWriter *writer2;
  SpCaptureReader *reader;
  SpCaptureFrameType type;
  GError *error = NULL;
  guint i;
  guint count;
  gint r;

  writer1 = sp_capture_writer_new ("writer1.syscap", 0);
  writer2 = sp_capture_writer_new ("writer2.syscap", 0);

  for (i = 0; i < 1000; i++)
    sp_capture_writer_add_timestamp (writer1, SP_CAPTURE_CURRENT_TIME, -1, -1);

  r = sp_capture_writer_flush (writer1);
  g_assert_cmpint (r, ==, TRUE);

  g_clear_pointer (&writer1, sp_capture_writer_unref);

  reader = sp_capture_reader_new ("writer1.syscap", &error);
  g_assert_no_error (error);
  g_assert (reader != NULL);

  /* advance to the end of the reader to non-start boundary for fd */

  for (i = 0; i < 1000; i++)
    {
      const SpCaptureTimestamp *ts = sp_capture_reader_read_timestamp (reader);

      g_assert (ts != NULL);
      g_assert_cmpint (ts->frame.cpu, ==, -1);
      g_assert_cmpint (ts->frame.pid, ==, -1);
      g_assert_cmpint (ts->frame.time, >, 0);
    }

  r = sp_capture_reader_peek_type (reader, &type);
  g_assert_cmpint (r, ==, FALSE);

  r = sp_capture_reader_splice (reader, writer2, &error);
  g_assert_no_error (error);
  g_assert_cmpint (r, ==, TRUE);

  g_clear_pointer (&reader, sp_capture_reader_unref);
  g_clear_pointer (&writer2, sp_capture_writer_unref);

  reader = sp_capture_reader_new ("writer2.syscap", 0);

  for (i = 0; i < 1000; i++)
    {
      const SpCaptureTimestamp *ts = sp_capture_reader_read_timestamp (reader);

      g_assert (ts != NULL);
      g_assert_cmpint (ts->frame.cpu, ==, -1);
      g_assert_cmpint (ts->frame.pid, ==, -1);
      g_assert_cmpint (ts->frame.time, >, 0);
    }

  r = sp_capture_reader_peek_type (reader, &type);
  g_assert_cmpint (r, ==, FALSE);

  g_clear_pointer (&reader, sp_capture_reader_unref);

  reader = sp_capture_reader_new ("writer2.syscap", &error);
  g_assert_no_error (error);
  g_assert (reader != NULL);

  r = sp_capture_reader_save_as (reader, "writer3.syscap", &error);
  g_assert_no_error (error);
  g_assert_cmpint (r, ==, TRUE);

  g_clear_pointer (&reader, sp_capture_reader_unref);

  reader = sp_capture_reader_new ("writer3.syscap", &error);
  g_assert_no_error (error);
  g_assert (reader != NULL);

  count = 0;
  while (sp_capture_reader_skip (reader))
    count++;
  g_assert_cmpint (count, ==, 1000);

  g_clear_pointer (&reader, sp_capture_reader_unref);

  g_unlink ("writer1.syscap");
  g_unlink ("writer2.syscap");
  g_unlink ("writer3.syscap");
}

int
main (int argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/SpCapture/ReaderWriter", test_reader_basic);
  g_test_add_func ("/SpCapture/Writer/splice", test_writer_splice);
  g_test_add_func ("/SpCapture/Reader/splice", test_reader_splice);
  return g_test_run ();
}
