/* sysprof-profile.h
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

#include <gio/gio.h>

G_BEGIN_DECLS

#define SYSPROF_PROFILE_INSIDE
# include "sysprof-battery-charge.h"
# include "sysprof-cpu-usage.h"
# include "sysprof-diagnostic.h"
# include "sysprof-disk-usage.h"
# include "sysprof-energy-usage.h"
# include "sysprof-instrument.h"
# include "sysprof-malloc-tracing.h"
# include "sysprof-memory-usage.h"
# include "sysprof-network-usage.h"
# include "sysprof-power-profile.h"
# include "sysprof-profiler.h"
# include "sysprof-proxied-instrument.h"
# include "sysprof-recording.h"
# include "sysprof-sampler.h"
# include "sysprof-spawnable.h"
# include "sysprof-symbols-bundle.h"
# include "sysprof-system-logs.h"
# include "sysprof-tracer.h"
#undef SYSPROF_PROFILE_INSIDE

G_END_DECLS
