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

#include "ggd.h"

#include <string.h>
#include <ctype.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <ctpl/ctpl.h>
#include <geanyplugin.h>

#include "ggd-file-type.h"
#include "ggd-file-type-manager.h"
#include "ggd-utils.h"
#include "ggd-plugin.h"



/* wrapper for ctpl_parser_parse() that returns a string. Free with g_free() */
static gchar *
parser_parse_to_string (const CtplToken *tree,
                        CtplEnviron     *env,
                        GError         **error)
{
  GOutputStream  *ostream;
  gchar          *output = NULL;
  
  ostream = g_memory_output_stream_new (NULL, 0, g_try_realloc, NULL);
  if (ctpl_parser_parse (tree, env, ostream, error)) {
    GMemoryOutputStream  *memostream = G_MEMORY_OUTPUT_STREAM (ostream);
    gsize                 size;
    gsize                 data_size;
    
    output = g_memory_output_stream_get_data (memostream);
    size = g_memory_output_stream_get_size (memostream);
    data_size = g_memory_output_stream_get_data_size (memostream);
    if (size <= data_size) {
      gpointer newptr;
      
      newptr = g_try_realloc (output, size + 1);
      if (newptr) {
        output = newptr;
        size ++;
      } else {
        /* not enough memory, simulate a memory output stream error */
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_NO_SPACE,
                     _("Failed to resize memory output stream"));
        g_free (output);
        output = NULL;
      }
    }
    if (size > data_size) {
      output[data_size] = 0;
    }
    g_object_unref (ostream);
  }
  
  return output;
}

/* extracts arguments from @args according to the given filetype */
static CtplValue *
get_arg_list_from_string (GgdFileType  *ft,
                          const gchar  *args)
{
  CtplValue *arg_list = NULL;
  
  g_return_val_if_fail (args != NULL, NULL);
  
  if (ft->match_function_arguments) {
    GMatchInfo *match_info;
    
    /*g_debug ("Trying to match against \"%s\"", args);*/
    if (! g_regex_match (ft->match_function_arguments, args, 0, &match_info)) {
      msgwin_status_add (_("Argument parsing regular expression did not match "
                           "(argument list was: \"%s\")"), args);
    } else {
      arg_list = ctpl_value_new_array (CTPL_VTYPE_STRING, 0, NULL);
      
      while (g_match_info_matches (match_info)) {
        gchar  *word = g_match_info_fetch (match_info, 1);
        
        /*g_debug ("Found arg: '%s'", word);*/
        if (word) {
          ctpl_value_array_append_string (arg_list, word);
        }
        g_free (word);
        g_match_info_next (match_info, NULL);
      }
    }
    g_match_info_free (match_info);
  }
  
  return arg_list;
}

/* pushes @value into @env as @{symbol}_list */
static void
hash_table_env_push_list_cb (gpointer symbol,
                             gpointer value,
                             gpointer env)
{
  gchar *symbol_name;
  
  symbol_name = g_strconcat (symbol, "_list", NULL);
  ctpl_environ_push (env, symbol_name, value);
  g_free (symbol_name);
}

/* gets the environment for a particular tag */
static CtplEnviron *
get_env_for_tag (GgdFileType   *ft,
                 GgdDocSetting *setting,
                 GPtrArray     *tag_array,
                 const TMTag   *tag)
{
  CtplEnviron *env;
  GList       *children = NULL;
  
  env = ctpl_environ_new ();
  ctpl_environ_push_string (env, "symbol", tag->name);
  /* get arguments & return type if appropriate */
  if (tag->type & (tm_tag_function_t |
                   tm_tag_macro_with_arg_t |
                   tm_tag_prototype_t)) {
    gboolean    returns;
    CtplValue  *v;
    
    v = get_arg_list_from_string (ft, tag->atts.entry.arglist);
    if (v) {
      ctpl_environ_push (env, "argument_list", v);
      ctpl_value_free (v);
    }
    returns = ! (tag->atts.entry.var_type != NULL &&
                 /* C-style none return type hack */
                 strcmp ("void", tag->atts.entry.var_type) == 0);
    ctpl_environ_push_int (env, "returns", returns);
  }
  /* get direct children tags */
  children = ggd_tag_find_children (tag_array, tag, 0);
  if (setting->merge_children) {
    CtplValue *v;
    
    v = ctpl_value_new_array (CTPL_VTYPE_STRING, 0, NULL);
    while (children) {
      TMTag  *el = children->data;
      GList  *tmp = children;
      
      if (el->type & setting->matches) {
        ctpl_value_array_append_string (v, el->name);
      }
      
      children = g_list_next (children);
      g_list_free_1 (tmp);
    }
    ctpl_environ_push (env, "children", v);
    ctpl_value_free (v);
  } else {
    GHashTable  *vars;
    
    vars = g_hash_table_new_full (g_str_hash, g_str_equal,
                                  NULL, (GDestroyNotify)ctpl_value_free);
    while (children) {
      TMTag        *el        = children->data;
      const gchar  *type_name = ggd_tag_get_type_name (el);
      CtplValue    *v;
      GList        *tmp = children;
      
      if (el->type & setting->matches) {
        v = g_hash_table_lookup (vars, type_name);
        if (! v) {
          v = ctpl_value_new_array (CTPL_VTYPE_STRING, 0, NULL);
          g_hash_table_insert (vars, (gpointer)type_name, v);
        }
        ctpl_value_array_append_string (v, el->name);
      }
      
      children = g_list_next (children);
      g_list_free_1 (tmp);
    }
    /* insert children into the environment */
    g_hash_table_foreach (vars, hash_table_env_push_list_cb, env);
    g_hash_table_destroy (vars);
  }
  
  return env;
}

/* parses the template @tpl with the environment of @tag */
static gchar *
get_comment (GgdFileType   *ft,
             GgdDocSetting *setting,
             GPtrArray     *tag_array,
             const TMTag   *tag)
{
  gchar *comment = NULL;
  
  if (setting->template) {
    GError      *err = NULL;
    CtplEnviron *env;
    
    env = get_env_for_tag (ft, setting, tag_array, tag);
    ctpl_environ_merge (env, ft->user_env, FALSE);
    comment = parser_parse_to_string (setting->template, env, &err);
    if (! comment) {
      msgwin_status_add (_("Failed to build comment: %s"), err->message);
      g_error_free (err);
    }
  }
  
  return comment;
}

/* Adjusts line where insert a function documentation comment.
 * This function adjusts start line of a (C) function start to be sure it is
 * before the return type, even if style places return type in the line that
 * precede function name.
 */
static gint
adjust_function_start_line (ScintillaObject *sci,
                            const gchar     *func_name,
                            gint             line)
{
  gchar  *str;
  gsize   i;
  
  str = sci_get_line (sci, line);
  for (i = 0; isspace (str[i]); i++);
  if (strncmp (&str[i], func_name, strlen (func_name)) == 0) {
    /* function return type is not in the same line as function name */
    #if 0
    /* this solution is more excat but perhaps too much complex for the gain */
    gint pos;
    gchar c;
    
    pos = sci_get_position_from_line (sci, line) - 1;
    for (; (c = sci_get_char_at (sci, pos)); pos --) {
      if (c == '\n' || c == '\r')
        line --;
      else if (! isspace (c))
        break;
    }
    #else
    line --;
    #endif
  }
  g_free (str);
  
  return line;
}

/* adjusts the start line of a tag */
static gint
adjust_start_line (ScintillaObject *sci,
                   GPtrArray       *tags,
                   const TMTag     *tag,
                   gint             line)
{
  if (tag->type & (tm_tag_function_t | tm_tag_prototype_t |
                   tm_tag_macro_with_arg_t)) {
    line = adjust_function_start_line (sci, tag->name, line);
  }
  
  return line;
}

/* inserts the comment for @tag in @sci according to @setting */
static gboolean
do_insert_comment (ScintillaObject *sci,
                   GPtrArray       *tag_array,
                   const TMTag     *tag,
                   GgdFileType     *ft,
                   GgdDocSetting   *setting)
{
  gboolean  success = FALSE;
  gchar    *comment;
  
  comment = get_comment (ft, setting, tag_array, tag);
  if (comment) {
    gint pos;
    
    switch (setting->position) {
      case GGD_POS_AFTER:
        pos = sci_get_line_end_position (sci, tag->atts.entry.line - 1);
        break;
      
      case GGD_POS_BEFORE: {
        gint line;
        
        line = tag->atts.entry.line - 1;
        line = adjust_start_line (sci, tag_array, tag, line);
        pos = sci_get_position_from_line (sci, line);
        break;
      }
      
      case GGD_POS_CURSOR:
        pos = sci_get_current_position (sci);
        break;
    }
    sci_insert_text (sci, pos, comment);
    success = TRUE;
  }
  g_free (comment);
  
  return success;
}

/* Gets the #GgdDocSetting that applies for a given tag.
 * Since a policy may forward documenting to a parent, tag that actually applies
 * is returned in @real_tag. */
static GgdDocSetting *
get_setting_from_tag (GgdDocType   *doctype,
                      GPtrArray    *tag_array,
                      const TMTag  *tag,
                      const TMTag **real_tag)
{
  GgdDocSetting  *setting;
  gchar          *hierarchy;
  gint            nth_child;
  
  hierarchy = ggd_tag_resolve_type_hierarchy (tag_array, tag);
  setting = ggd_doc_type_resolve_setting (doctype, hierarchy, &nth_child);
  *real_tag = tag;
  if (setting) {
    for (; nth_child > 0; nth_child--) {
      *real_tag = ggd_tag_find_parent (tag_array, *real_tag);
    }
  }
  g_free (hierarchy);
  
  return setting;
}

/**
 * ggd_insert_comment:
 * @doc: The document in which insert the comment
 * @line: SCI's line for which generate a comment. Usually the current line.
 * 
 * Tries to insert a comment in a #GeanyDocument.
 * 
 * <warning>
 *   if tag list is not up-to-date, the result can be surprising
 * </warning>
 * 
 * Returns: %TRUE is a comment was added, %FALSE otherwise.
 */
gboolean
ggd_insert_comment (GeanyDocument  *doc,
                    gint            line,
                    const gchar    *doc_type)
{
  gboolean          success = FALSE;
  ScintillaObject  *sci;
  const TMTag      *tag;
  GPtrArray        *tag_array;
  
  sci = doc->editor->sci;
  tag_array = doc->tm_file->tags_array;
  tag = ggd_tag_find_from_line (tag_array, line + 1 /* it is a SCI line */);
  if (! tag || (tag->type & tm_tag_file_t)) {
    msgwin_status_add (_("No valid tag for line %d"), line);
  } else {
    GgdFileType *ft;
    
    ft = ggd_file_type_manager_get_file_type (doc->file_type->id);
    if (ft) {
      GgdDocType *doctype;
      
      doctype = ggd_file_type_get_doc (ft, doc_type);
      if (! doctype) {
        msgwin_status_add (_("No documentation type \"%s\" for language \"%s\""),
                           doc_type, doc->file_type->name);
      } else {
        GgdDocSetting *setting;
        
        setting = get_setting_from_tag (doctype, tag_array, tag, &tag);
        if (setting) {
          success = do_insert_comment (sci, tag_array, tag, ft, setting);
        }
      }
    }
  }
  
  return success;
}

/**
 * ggd_insert_all_comments:
 * @doc: A #GeanyDocument for which insert the comments
 * @doc_type: Documentation type identifier
 * 
 * Tries to insert a comment for each symbol of a document.
 * 
 * Returns: %TRUE on full success, %FALSE otherwise.
 */
gboolean
ggd_insert_all_comments (GeanyDocument *doc,
                         const gchar   *doc_type)
{
  gboolean      success = FALSE;
  GgdFileType  *ft;
  
  g_return_val_if_fail (DOC_VALID (doc), FALSE);
  
  ft = ggd_file_type_manager_get_file_type (doc->file_type->id);
  if (ft) {
    GgdDocType *doctype;
    
    doctype = ggd_file_type_get_doc (ft, doc_type);
    if (! doctype) {
      msgwin_status_add (_("No documentation type \"%s\" for language \"%s\""),
                         doc_type, doc->file_type->name);
    } else {
      GPtrArray        *tag_array;
      ScintillaObject  *sci = doc->editor->sci;
      const TMTag      *tag;
      guint             i;
      GHashTable       *tag_done_table; /* keeps the list of documented tags.
                                         * Useful since documenting a tag might
                                         * actually document another one */
      
      success = TRUE;
      tag_array = doc->tm_file->tags_array;
      tag_done_table = g_hash_table_new (NULL, NULL);
      /* sort the tags to be sure to insert by the end of the document, then we
       * don't modify the element's position of tags we'll work on */
      ggd_tag_sort_by_line (tag_array, GGD_SORT_DESC);
      sci_start_undo_action (sci);
      GGD_PTR_ARRAY_FOR (tag_array, i, tag) {
        GgdDocSetting  *setting;
        
        setting = get_setting_from_tag (doctype, tag_array, tag, &tag);
        if (setting && ! g_hash_table_lookup (tag_done_table, tag)) {
          if (! do_insert_comment (sci, tag_array, tag, ft, setting)) {
            success = FALSE;
            break;
          } else {
            g_hash_table_insert (tag_done_table, (gpointer)tag, (gpointer)tag);
          }
        }
      }
      sci_end_undo_action (sci);
      g_hash_table_destroy (tag_done_table);
    }
  }
  
  return success;
}
