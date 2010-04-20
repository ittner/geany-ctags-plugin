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


#include "ggd-tag-utils.h"

#include <geanyplugin.h>
#include <glib.h>

#include "ggd-plugin.h" /* to access Geany data/funcs */


/* a `for` header to walk on an GPtrArray
 * @array: the array to traverse
 * @idx: A guint variable to use as iterator (may be modified)
 * @el: a TMTag pointer to fill with the current element in the array
 * 
 * usage:
 * 
 * guint  i;
 * TMTag *tag;
 * 
 * PTR_ARRAY_FOR (ptr_array, i, tag) {
 *   // use @tag here...
 * }
 */
#define PTR_ARRAY_FOR(array, idx, el) \
  for ((idx) = 0; ((el) = g_ptr_array_index ((array), (idx)), \
                   (idx) < (array)->len); (idx)++)


/* Compare function for g_ptr_array_sort() to compare two TMTag by their
 * lines */
static gint
tag_cmp_by_line (gconstpointer a,
                 gconstpointer b)
{
  const TMTag *t1 = *((const TMTag**)a);
  const TMTag *t2 = *((const TMTag**)b);
  gint rv;
  
  if (t1->type & tm_tag_file_t || t2->type & tm_tag_file_t) {
    rv = 0;
  } else {
    if (t1->atts.entry.line > t2->atts.entry.line) {
      rv = 1;
    } else if (t1->atts.entry.line < t2->atts.entry.line) {
      rv = -1;
    } else {
      rv = 0;
    }
  }
  
  return rv;
}

void
ggd_tag_sort_by_line (GPtrArray *tags)
{
  g_return_if_fail (tags != NULL);
  
  g_ptr_array_sort (tags, tag_cmp_by_line);
}

TMTag *
ggd_tag_find_from_line (const GPtrArray  *tags,
                        gulong            line)
{
  TMTag    *tag = NULL;
  TMTag    *el;
  guint     i;
  
  g_return_val_if_fail (tags != NULL, NULL);
  
  PTR_ARRAY_FOR (tags, i, el) {
    if (! (el->type & tm_tag_file_t)) {
      if (el->atts.entry.line <= line &&
          (! tag || el->atts.entry.line > tag->atts.entry.line)) {
        tag = el;
      }
    }
  }
  
  return tag;
}

TMTag *
ggd_tag_find_at_current_pos (GeanyDocument *doc)
{
  TMTag *tag = NULL;
  
  if (doc && doc->tm_file) {
    tag = ggd_tag_find_from_line (doc->tm_file->tags_array,
                                  sci_get_current_line (doc->editor->sci) + 1);
  }
  
  return tag;
}

TMTag *
ggd_tag_find_parent (const GPtrArray *tags,
                     const TMTag     *child)
{
  TMTag *tag = NULL;
  
  g_return_val_if_fail (tags != NULL, NULL);
  g_return_val_if_fail (child != NULL, NULL);
  
  if (! child->atts.entry.scope) {
    /* tag has no parent, we're done */
  } else {
    gchar        *parent_scope = NULL;
    const gchar  *parent_name;
    const gchar  *tmp;
    guint         i;
    TMTag        *el;
    
    /* scope is of the form a::b::c */
    parent_name = child->atts.entry.scope;
    while ((tmp = strstr (parent_name, "::")) != NULL) {
      parent_name = &tmp[2];
    }
    /* if parent have scope */
    if (parent_name != child->atts.entry.scope) {
      /* the parent scope is the "dirname" of the child's scope */
      parent_scope = g_strndup (child->atts.entry.scope,
                                parent_name - child->atts.entry.scope - 2);
    }
    /*g_debug ("%s: parent_name = %s", G_STRFUNC, parent_name);
    g_debug ("%s: parent_scope = %s", G_STRFUNC, parent_scope);*/
    PTR_ARRAY_FOR (tags, i, el) {
      if (! (el->type & tm_tag_file_t) &&
          (utils_str_equal (el->name, parent_name) &&
           utils_str_equal (el->atts.entry.scope, parent_scope))) {
        tag = el;
        break;
      }
    }
    g_free (parent_scope);
  }
  
  return tag;
}

static const struct {
  const TMTagType     type;
  const gchar *const  name;
} GGD_tag_types[] = {
  { tm_tag_class_t,           "class"     },
  { tm_tag_enum_t,            "enum"      },
  { tm_tag_enumerator_t,      "enumval"   },
  { tm_tag_field_t,           "field"     },
  { tm_tag_function_t,        "function"  },
  { tm_tag_interface_t,       "interface" },
  { tm_tag_member_t,          "member"    },
  { tm_tag_method_t,          "method"    },
  { tm_tag_namespace_t,       "namespace" },
  { tm_tag_package_t,         "package"   },
  { tm_tag_prototype_t,       "prototype" },
  { tm_tag_struct_t,          "struct"    },
  { tm_tag_typedef_t,         "typedef"   },
  { tm_tag_union_t,           "union"     },
  { tm_tag_variable_t,        "variable"  },
  { tm_tag_externvar_t,       "extern"    },
  { tm_tag_macro_t,           "define"    },
  { tm_tag_macro_with_arg_t,  "macro"     },
  { tm_tag_file_t,            "file"      }
};

/* Tries to convert @type to string. Returns %NULL on failure */
const gchar *
ggd_tag_type_get_name (TMTagType type)
{
  guint i;
  
  for (i = 0; i < G_N_ELEMENTS (GGD_tag_types); i++) {
    if (GGD_tag_types[i].type == type) {
      return GGD_tag_types[i].name;
    }
  }
  
  return NULL;
}

/* Tries to convert @name to tag type. Returns 0 on failure */
TMTagType
ggd_tag_type_from_name (const gchar *name)
{
  guint i;
  
  g_return_val_if_fail (name != NULL, 0);
  
  for (i = 0; i < G_N_ELEMENTS (GGD_tag_types); i++) {
    if (utils_str_equal (GGD_tag_types[i].name, name)) {
      return GGD_tag_types[i].type;
    }
  }
  
  return 0;
}

/* Gets the name of a tag type */
const gchar *
ggd_tag_get_type_name (const TMTag *tag)
{
  g_return_val_if_fail (tag, NULL);
  
  return ggd_tag_type_get_name (tag->type);
}

/**
 * ggd_tag_resolve_type_hierarchy:
 * @tags: The tag list that contains @tag
 * @tag: A #TMTag to which get the type hierarchy
 * 
 * Gets the type hierarchy of a tag as a string, each element separated by a
 * dot.
 * 
 * FIXME: perhaps we should use array of type's ID rather than a string.
 * FIXME: perhaps drop recursion
 * 
 * Returns: the tag's type hierarchy or %NULL if invalid.
 */
gchar *
ggd_tag_resolve_type_hierarchy (const GPtrArray *tags,
                                const TMTag     *tag)
{
  gchar *scope = NULL;
  
  g_return_val_if_fail (tags != NULL, NULL);
  g_return_val_if_fail (tag != NULL, NULL);
  
  if (tag->type & tm_tag_file_t) {
    g_critical (_("Invalid tag"));
  } else {
    TMTag *parent_tag;
    
    parent_tag = ggd_tag_find_parent (tags, tag);
    scope = g_strdup (ggd_tag_get_type_name (tag));
    if (parent_tag) {
      gchar *parent_scope;
      
      parent_scope = ggd_tag_resolve_type_hierarchy (tags, parent_tag);
      if (! parent_scope) {
        /*g_debug ("no parent scope");*/
      } else {
        gchar *tmp;
        
        tmp = g_strconcat (parent_scope, ".", scope, NULL);
        g_free (scope);
        scope = tmp;
        g_free (parent_scope);
      }
    }
  }
  
  return scope;
}

/**
 * ggd_tag_find_from_name:
 * @tags: A #GPtrArray of tags
 * @name: the name of the tag to search for
 * 
 * Gets the tag named @name in @tags
 * 
 * Returns: The #TMTag named @name, or %NULL if none matches
 */
TMTag *
ggd_tag_find_from_name (const GPtrArray *tags,
                        const gchar     *name)
{
  TMTag  *tag = NULL;
  guint   i;
  TMTag  *el;
  
  g_return_val_if_fail (tags != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);
  
  PTR_ARRAY_FOR (tags, i, el) {
    if (! (el->type & tm_tag_file_t) &&
        utils_str_equal (el->name, name)) {
      tag = el;
      break;
    }
  }
  
  return tag;
}


/*
 * scope_child_matches:
 * @a: parent scope
 * @b: child scope
 * @maxdepth: maximum sub-child level that matches, or < 0 for all to match
 * 
 * Checks if scope @b is inside scope @a. @maxdepth make possible to only match
 * child scope if it have less than @maxdepth parents before scope @a.
 * E.g., with a maximum depth of 1, only direct children will match.
 * 
 * Returns: %TRUE if matches, %FALSE otherwise.
 */
static gboolean
scope_child_matches (const gchar *a,
                     const gchar *b,
                     gint         maxdepth)
{
  gboolean matches = FALSE;
  
  if (a && b) {
    for (; *a && *b && *a == *b; a++, b++);
    if (! *a /* we're at the end of the prefix and it matched */) {
      if (maxdepth < 0) {
        if (! *b || (b[0] == ':' && b[1] == ':')) {
          matches = TRUE;
        }
      } else {
        while (! matches && maxdepth >= 0) {
          const gchar *tmp;
          
          tmp = strstr (b, "::");
          if (tmp) {
            b = &tmp[2];
            maxdepth --;
          } else {
            if (! *b) {
              matches = TRUE;
            }
            break;
          }
        }
      }
    }
  }
  
  return matches;
}


/**
 * ggd_tag_find_children:
 * @tags: Array of tags that contains @parent
 * @parent: Tag for which get children
 * @depth: Maximum depth for children to be found (< 0 means infinite)
 * 
 * 
 * 
 * Returns: The list of found children
 */
GList *
ggd_tag_find_children (const GPtrArray *tags,
                       const TMTag     *parent,
                       gint             depth)
{
  GList  *children = NULL;
  guint   i;
  TMTag  *el;
  gchar  *fake_scope;
  
  g_return_val_if_fail (tags != NULL, NULL);
  g_return_val_if_fail (parent != NULL, NULL);
  
  /* FIXME: sort by line? */
  if (parent->atts.entry.scope) {
    fake_scope = g_strconcat (parent->atts.entry.scope, parent->name, NULL);
  } else {
    fake_scope = g_strdup (parent->name);
  }
  PTR_ARRAY_FOR (tags, i, el) {
    if (scope_child_matches (fake_scope, el->atts.entry.scope, depth)) {
      children = g_list_append (children, el);
    }
  }
  g_free (fake_scope);
  
  return children;
}
