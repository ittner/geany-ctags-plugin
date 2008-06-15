#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# WAF build script for geany-plugins
#
# Copyright 2008 Enrico Tröger <enrico(dot)troeger(at)uvena(dot)de>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# $Id$

"""
Waf build script for Geany Plugins.

If you want to add your plugin to this build system, simply add a Plugin object
to the plugin list below by adding a new line and passing the appropiate
values to the Plugin constructor. Everything below doesn't need to be changed.

If you need additional checks for header files, functions in libraries or
need to check for library packages (using pkg-config), please ask Enrico
before committing changes. Thanks.

Requires WAF SVN r3624 (or later) and Python 2.4 (or later).
"""


import Params, Configure, Runner
import sys, os, subprocess


APPNAME = 'geany-plugins'
VERSION = '0.1'

srcdir = '.'
blddir = 'build'


class Plugin:
    def __init__(self, n, s, i, v=VERSION, l=[]):
        self.name = n
        self.sources = s
        self.includes = i # do not include '.'
        self.version = v
        self.libs = l # a list of lists of libs and their versions, e.g. [ [ 'gtk', '2.6' ], [ 'gtkspell', '0.1' ] ]


# add a new element for your plugin
plugins = [
    Plugin('backupcopy',
         [ 'backupcopy/src/backupcopy.c' ], # source files
         [ 'backupcopy', 'backupcopy/src' ], # include dirs
         '0.2'),
    Plugin('geanysendmail',
           [ 'geanysendmail/src/geanysendmail.c' ], # source files
           [ 'geanysendmail', 'geanysendmail/src' ], # include dirs
           '0.4git'),
    Plugin('geanydoc',
         [ 'geanydoc/src/config.c', 'geanydoc/src/geanydoc.c' ], # source files
         [ 'geanydoc', 'geanydoc/src' ], # include dirs
         '0.3'),
    Plugin('instantsave',
         [ 'instantsave/src/instantsave.c' ], # source files
         [ 'instantsave', 'instantsave/src/' ], # include dirs
         '0.2'),
    Plugin('spellcheck',
         [ 'spellcheck/src/spellcheck.c' ], # source files
         [ 'spellcheck', 'spellcheck/src' ], # include dirs
         '0.2', [ [ 'enchant', '1.3' ] ])
]



def configure(conf):
    def conf_get_svn_rev():
        try:
            p = subprocess.Popen(['svn', 'info', '--non-interactive'], stdout=subprocess.PIPE, \
                    stderr=subprocess.STDOUT, close_fds=False, env={'LANG' : 'C'})
            stdout = p.communicate()[0]

            if p.returncode == 0:
                lines = stdout.splitlines(True)
                for line in lines:
                    if line.startswith('Last Changed Rev'):
                        key, value = line.split(': ', 1)
                        return value.strip()
            return '-1'
        except:
            return '-1'

    def conf_get_pkg_ver(pkgname):
        ret = os.popen('PKG_CONFIG_PATH=$PKG_CONFIG_PATH pkg-config --modversion %s' % pkgname).read().strip()
        if ret:
            return ret
        else:
            return '(unknown)'

    def conf_define_from_opt(define_name, opt_name, default_value, quote=1):
        if opt_name:
            if isinstance(opt_name, bool):
                opt_name = 1
            conf.define(define_name, opt_name, quote)
        elif default_value:
            conf.define(define_name, default_value, quote)


    conf.check_tool('compiler_cc intltool')
    conf.check_pkg('gtk+-2.0', destvar='GTK', vnum='2.6.0')
    conf.check_pkg('geany', destvar='GEANY', vnum='0.15')

    enabled_plugins = []
    if Params.g_options.enable_plugins:
		for p_name in Params.g_options.enable_plugins.split(','):
			enabled_plugins.append(p_name.strip())
    else:
        for p in plugins:
            enabled_plugins.append(p.name)

    # check for plugin deps
    for p in plugins:
        if p.name in enabled_plugins:
            for l in p.libs:
                if not conf.check_pkg(l[0], destvar=l[0].upper(), vnum=l[1], mandatory=False):
                    enabled_plugins.remove(p.name)

    conf_define_from_opt('LIBDIR', Params.g_options.libdir, conf.env['PREFIX'] + '/lib')

    svn_rev = conf_get_svn_rev()
    conf.define('ENABLE_NLS', 1)
    conf.define('REVISION', svn_rev, 1)

    # write a config.h for each plugin
    for p in plugins:
        if p.name in enabled_plugins:
            conf.define('VERSION', p.version, 1)
            conf.define('PACKAGE', p.name, 1)
            conf.define('GETTEXT_PACKAGE', p.name, 1)
            conf.write_config_header(os.path.join(p.name, 'config.h'))

    Params.pprint('BLUE', 'Summary:')
    print_message('Install Geany Plugins ' + VERSION + ' in', conf.env['PREFIX'])
    print_message('Using GTK version', conf_get_pkg_ver('gtk+-2.0'))
    print_message('Using Geany version', conf_get_pkg_ver('geany'))
    print_message('Using Enchant version', conf_get_pkg_ver('enchant'))
    if svn_rev != '-1':
        print_message('Compiling Subversion revision', svn_rev)
        conf.env.append_value('CCFLAGS', '-g -O0') # -DGEANY_DISABLE_DEPRECATED')

    print_message('Plugins to compile', ' '.join(enabled_plugins))

    conf.env.append_value('enabled_plugins', enabled_plugins)
    conf.env.append_value('CCFLAGS', '-DHAVE_CONFIG_H')


def set_options(opt):
    opt.tool_options('compiler_cc')
    opt.tool_options('intltool')

    # Paths
    opt.add_option('--libdir', type='string', default='',
        help='object code libraries', dest='libdir')
    # Actions
    opt.add_option('--update-po', action='store_true', default=False,
        help='update the message catalogs for translation', dest='update_po')
    opt.add_option('--list-plugins', action='store_true', default=False,
        help='list plugins which can be built', dest='list_plugins')

    # enable-plugins should only be used for configure
    if 'configure' in sys.argv:
        opt.add_option('--enable-plugins', action='store', default='',
            help='plugins to be built [plugins in CSV format, e.g. "%(1)s,%(2)s"]' % \
            { '1' : plugins[0].name, '2' : plugins[1].name }, dest='enable_plugins')


def build(bld):
    for p in plugins:
        if not p.name in bld.env['enabled_plugins']:
            continue;

        libs = 'GTK GEANY' # common for all plugins
        for l in p.libs:   # add plugin specific libs
            libs += ' %s' % l[0].upper()

        obj                         = bld.new_task_gen('cc', 'shlib')
        obj.source                  = p.sources
        obj.includes                = p.includes
        obj.env['shlib_PATTERN']    = '%s.so'
        obj.target                  = p.name
        obj.uselib                  = libs
        obj.inst_var                = 'LIBDIR'
        obj.inst_dir                = '/geany/'

        if os.path.exists(os.path.join(p.name, 'po')):
            obj         = bld.new_task_gen('intltool_po')
            obj.podir   = os.path.join(p.name, 'po')
            obj.appname = p.name


def init():
    if Params.g_options.list_plugins:
        Params.pprint('GREEN', \
            'The following targets can be chosen with the --enable-plugins option:')
        for p in plugins:
            print p.name,
        Params.pprint('GREEN', \
    '\nTo compile only "%(1)s" and "%(2)s", use "./waf configure --enable-plugins=%(1)s,%(2)s".' % \
            { '1' : plugins[0].name, '2' : plugins[1].name } )
        sys.exit(0)


def shutdown():
    if Params.g_options.update_po:
        # the following code is based on code from midori's WAF script, thanks
        for p in plugins:
            if not p.name in Params.g_build.env['enabled_plugins']:
                continue;
            dir = os.path.join(p.name, 'po')
            os.chdir(dir)
            try:
                try:
                    size_old = os.stat(p.name + '.pot').st_size
                except:
                    size_old = 0
                subprocess.call(['intltool-update', '--pot'])
                size_new = os.stat(p.name + '.pot').st_size
                if size_new != size_old:
                    Params.pprint('CYAN', 'Updated POT file for %s.' % p.name)
                    launch('intltool-update -r', 'Updating translations for %s' % p.name, 'CYAN')
                else:
                    Params.pprint('CYAN', 'POT file is up to date for %s.' % p.name)
            except:
                Params.pprint('RED', 'Failed to generate pot file for %s.' % p.name)
            os.chdir(os.path.join('..', '..'))



# Simple function to execute a command and print its exit status
def launch(command, status, success_color='GREEN'):
    ret = 0
    Params.pprint(success_color, status)
    try:
        ret = subprocess.call(command.split())
    except:
        ret = 1

    if ret != 0:
        Params.pprint('RED', status + ' failed')

    return ret


def print_message(msg, result, color = 'GREEN'):
    Configure.g_maxlen = max(Configure.g_maxlen, len(msg))
    print "%s :" % msg.ljust(Configure.g_maxlen),
    Params.pprint(color, result)
    Runner.print_log(msg, '\n\n')

