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

#ifndef H_GGD_UTILS
#define H_GGD_UTILS

#include <glib.h>

G_BEGIN_DECLS


enum _GgdPerms {
  GGD_PERM_R    = 1 << 0,
  GGD_PERM_W    = 1 << 1,
  GGD_PERM_RW   = GGD_PERM_R | GGD_PERM_W
};

typedef enum _GgdPerms GgdPerms;

gchar          *ggd_get_config_file             (const gchar *name,
                                                 const gchar *section,
                                                 GgdPerms     perms_req,
                                                 GError     **error);


G_END_DECLS

#endif /* guard */
