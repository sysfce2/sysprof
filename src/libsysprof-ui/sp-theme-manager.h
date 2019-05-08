/* sp-theme-manager.h
 *
 * Copyright 2016-2019 Christian Hergert <chergert@redhat.com>
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

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define SP_TYPE_THEME_MANAGER (sp_theme_manager_get_type())

G_GNUC_INTERNAL
G_DECLARE_FINAL_TYPE (SpThemeManager, sp_theme_manager, SP, THEME_MANAGER, GObject)

G_GNUC_INTERNAL
SpThemeManager *sp_theme_manager_get_default       (void);
G_GNUC_INTERNAL
void            sp_theme_manager_unregister        (SpThemeManager *self,
                                                    guint           registration_id);
G_GNUC_INTERNAL
guint           sp_theme_manager_register_resource (SpThemeManager *self,
                                                    const gchar    *theme_name,
                                                    const gchar    *variant,
                                                    const gchar    *resource);

G_END_DECLS
