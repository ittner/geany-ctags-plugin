/*
 *  
 *  Copyright © 2010 Colomban "Ban" Wendling <ban@herbesfolles.org>
 *  
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *  
 */

#ifndef H_GGD_TAG_UTILS
#define H_GGD_TAG_UTILS

#include <glib.h>
#include <geanyplugin.h>

G_BEGIN_DECLS


TMTag        *ggd_tag_find_from_line          (GPtrArray  *tags,
                                               gulong      line);
TMTag        *ggd_tag_find_at_current_pos     (GeanyDocument *doc);
TMTag        *ggd_tag_find_parent             (GPtrArray   *tags,
                                               const TMTag *child);
GList        *ggd_tag_find_children           (GPtrArray   *tags,
                                               const TMTag *parent,
                                               gint         depth);
gchar        *ggd_tag_resolve_type_hierarchy  (GPtrArray   *tags,
                                               const TMTag *tag);
TMTag        *ggd_tag_find_from_name          (GPtrArray   *tags,
                                               const gchar *name);
const gchar  *ggd_tag_get_type_name           (const TMTag *tag);
const gchar  *ggd_tag_type_get_name           (TMTagType  type);
TMTagType     ggd_tag_type_from_name          (const gchar *name);


G_END_DECLS

#endif /* guard */
