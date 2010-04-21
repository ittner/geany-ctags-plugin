/*
 *  
 *  Copyright © 2009 Colomban "Ban" Wendling <ban@herbesfolles.org>
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

#ifndef H_GGD_PLUGIN
#define H_GGD_PLUGIN

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <geanyplugin.h>

G_BEGIN_DECLS


#define GGD_PLUGIN_ONAME  geanygendoc
#define GGD_PLUGIN_CNAME  PACKAGE_TARNAME
#define GGD_PLUGIN_NAME   "GeanyGenDoc"


/* pretty funny hack, because Geany doesn't use a shared library for it's API,
 * then we need that global function table pointer, that Geany gives at plugin
 * loading time */
extern  GeanyPlugin      *geany_plugin;
extern  GeanyData        *geany_data;
extern  GeanyFunctions   *geany_functions;


G_END_DECLS

#endif /* guard */