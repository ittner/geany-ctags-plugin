/*
 *      latexenvironments.h
 *
 *      Copyright 2009 Frank Lanitz <frank(at)frank(dot)uvena(dot)de>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#ifndef LATEXENVIRONMENTS_H
#define LATEXENVIRONMENTS_H

#include "geanylatex.h"

enum {
    ENVIRONMENT_CAT_DUMMY = 0,
    ENVIRONMENT_CAT_MAX
};

extern SubMenuTemplate glatex_environment_array[];

extern CategoryName glatex_environment_cat_names[];

void glatex_insert_environment(gchar *environment);

#endif
