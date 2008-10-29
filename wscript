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

Requires WAF 1.5 (SVN r4695 or later) and Python 2.4 (or later).
"""


import Build, Configure, Options, Runner, Utils
import sys, os, subprocess


APPNAME = 'geany-plugins'
VERSION = '0.1'

srcdir = '.'
blddir = '_build_'


class Plugin:
	def __init__(self, n, s, i, v=VERSION, l=[]):
		self.name = n
		self.sources = s
		self.includes = i # do not include '.'
		self.version = v
		self.libs = l # a list of lists of libs and their versions, e.g. [ [ 'gtk', '2.6' ],
		              # [ 'gtkspell-2.0', '2.0', False ] ], the third argument defines whether
		              # the dependency is mandatory


# add a new element for your plugin
plugins = [
	Plugin('geanydbg',
		 [ 'geanydbg/src/dbg.c' ], # source files
		 [ 'geanydbg', 'geanydbg/src' ], # include dirs
		 '0.1'),
	Plugin('geanydebug',
		 map(lambda x: "geanydebug/src/" + x, ['gdb-io-break.c',
		   'gdb-io-envir.c', 'gdb-io-frame.c', 'gdb-io-read.c', 'gdb-io-run.c',
		   'gdb-io-stack.c', 'gdb-lex.c', 'gdb-ui-break.c', 'gdb-ui-envir.c',
		   'gdb-ui-frame.c',  'gdb-ui-locn.c', 'gdb-ui-main.c',
		   'geanydebug.c']), # source files
		 [ 'geanydebug', 'geanydebug/src' ], # include dirs
		 '0.1'),
	Plugin('geanysendmail',
		 [ 'geanysendmail/src/geanysendmail.c' ], # source files
		 [ 'geanysendmail', 'geanysendmail/src' ], # include dirs
		 '0.5svn'),
	Plugin('geanydoc',
		 [ 'geanydoc/src/config.c', 'geanydoc/src/geanydoc.c' ], # source files
		 [ 'geanydoc', 'geanydoc/src' ], # include dirs
		 '0.3'),
	Plugin('geanylatex',
		 [ 'geanylatex/src/latexencodings.c', 'geanylatex/src/geanylatex.c',
		   'geanylatex/src/letters.c' ],
		 [ 'geanylatex' ], # include dirs
		 '0.2'),
	Plugin('geanylua',
		 [ 'geanylua/geanylua.c' ], # the other source files are listed in build_lua()
		 [ 'geanylua' ], # include dirs
		 # maybe you need to modify the package name of Lua, try one of these: lua5.1 lua51 lua-5.1
		 '0.7.0', [ [ 'lua', '5.1', True ] ]),
	Plugin('geanyprj',
		 [ 'geanyprj/src/geanyprj.c', 'geanyprj/src/menu.c', 'geanyprj/src/project.c',
		   'geanyprj/src/sidebar.c', 'geanyprj/src/utils.c', 'geanyprj/src/xproject.c' ],
		 [ 'geanyprj', 'geanyprj/src' ], # include dirs
		 '0.4'),
	Plugin('geanyvc',
		 [ 'geanyvc/geanyvc.c', 'geanyvc/utils.c', 'geanyvc/externdiff.c',
		   'geanyvc/vc_git.c', 'geanyvc/vc_cvs.c', 'geanyvc/vc_svn.c',
		   'geanyvc/vc_svk.c', 'geanyvc/vc_bzr.c', 'geanyvc/vc_hg.c' ],
		 [ 'geanyvc' ], # include dirs
		 '0.4', [ [ 'gtkspell-2.0', '2.0', False ] ]),
	Plugin('spellcheck',
		 [ 'spellcheck/src/gui.c', 'spellcheck/src/scplugin.c', 'spellcheck/src/speller.c' ], # source files
		 [ 'spellcheck', 'spellcheck/src' ], # include dirs
		 '0.3', [ [ 'enchant', '1.3', True ] ])
]



def configure(conf):
	def conf_get_svn_rev():
		# try GIT
		if os.path.exists('.git'):
			cmds = [ 'git svn find-rev HEAD 2>/dev/null',
					 'git svn find-rev origin/trunk 2>/dev/null',
					 'git svn find-rev trunk 2>/dev/null',
					 'git svn find-rev master 2>/dev/null' ]
			for c in cmds:
				try:
					stdout = Utils.cmd_output(c)
					if stdout:
						return stdout.strip()
				except:
					pass
		# try SVN
		elif os.path.exists('.svn'):
			try:
				stdout = Utils.cmd_output('svn info --non-interactive', {'LANG' : 'C'})
				lines = stdout.splitlines(True)
				for line in lines:
					if line.startswith('Last Changed Rev'):
						key, value = line.split(': ', 1)
						return value.strip()
			except:
				pass
		else:
			pass
		return '-1'

	def conf_define_from_opt(define_name, opt_name, default_value, quote=1):
		if opt_name:
			if isinstance(opt_name, bool):
				opt_name = 1
			conf.define(define_name, opt_name, quote)
		elif default_value:
			conf.define(define_name, default_value, quote)


	conf.check_tool('compiler_cc intltool')

	conf.check_cfg(package='gtk+-2.0', atleast_version='2.6.0', uselib_store='GTK', mandatory=True)
	conf.check_cfg(package='gtk+-2.0', args='--cflags --libs', uselib_store='GTK')
	conf.check_cfg(package='geany', atleast_version='0.15', mandatory=True)
	conf.check_cfg(package='geany', args='--cflags --libs')

	gtk_version = conf.check_cfg(modversion='gtk+-2.0') or 'Unknown'
	geany_version = conf.check_cfg(modversion='geany') or 'Unknown'

	enabled_plugins = []
	if Options.options.enable_plugins:
		for p_name in Options.options.enable_plugins.split(','):
			enabled_plugins.append(p_name.strip())
	else:
		skipped_plugins = Options.options.skip_plugins.split(',')
		for p in plugins:
				if not p.name in skipped_plugins:
					enabled_plugins.append(p.name)

	# remove enabled but not existent plugins
	for p in plugins:
		if p.name in enabled_plugins and not os.path.exists(p.name):
			enabled_plugins.remove(p.name)

	# check for plugin deps
	for p in plugins:
		if p.name in enabled_plugins:
			for l in p.libs:
				uselib = Utils.quote_define_name(l[0])
				conf.check_cfg(package=l[0], uselib_store=uselib, atleast_version=l[1])
				if not conf.env['HAVE_%s' % uselib] == 1:
					if l[2]:
						enabled_plugins.remove(p.name)
				else:
					conf.check_cfg(package=l[0], args='--cflags --libs', uselib_store=uselib)

	conf_define_from_opt('LIBDIR', Options.options.libdir, conf.env['PREFIX'] + '/lib')
	# get and define Geany's libdir for use as plugin binary installation dir
	libdir = conf.check_cfg(package='geany', args='--variable=libdir')
	if libdir:
		conf.define('GEANY_LIBDIR', libdir.strip(), 1)
	else:
		conf.define('GEANY_LIBDIR', conf.env['LIBDIR'], 1)

	svn_rev = conf_get_svn_rev()
	conf.define('ENABLE_NLS', 1)
	conf.define('REVISION', svn_rev, 1)

	# write a config.h for each plugin
	for p in plugins:
		if p.name in enabled_plugins:
			if p.name == 'geanyvc' and conf.env['HAVE_GTKSPELL_2_0'] == 1:
				# hack for GeanyVC
				conf.define('USE_GTKSPELL', 1);
			conf.define('VERSION', p.version, 1)
			conf.define('PACKAGE', p.name, 1)
			conf.define('GETTEXT_PACKAGE', p.name, 1)
			conf.write_config_header(os.path.join(p.name, 'config.h'))

	Utils.pprint('BLUE', 'Summary:')
	print_message(conf, 'Install Geany Plugins ' + VERSION + ' in', conf.env['PREFIX'])
	print_message(conf, 'Using GTK version', gtk_version)
	print_message(conf, 'Using Geany version', geany_version)
	if svn_rev != '-1':
		print_message(conf, 'Compiling Subversion revision', svn_rev)
		conf.env.append_value('CCFLAGS', '-g -O0') # -DGEANY_DISABLE_DEPRECATED')

	print_message(conf, 'Plugins to compile', ' '.join(enabled_plugins))

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
		opt.add_option('--skip-plugins', action='store', default='',
			help='plugins which should not be built, ignored when --enable-plugins is set, same format as --enable-plugins' % \
			{ '1' : plugins[0].name, '2' : plugins[1].name }, dest='skip_plugins')

#~ future code
#~ def error_handler(self, tsk):
	#~ print "haha, %r failed" % tsk
	#~ self.error = 0

def build(bld):
	for p in plugins:
		if not p.name in bld.env['enabled_plugins']:
			continue;

		libs = 'GTK GEANY' # common for all plugins
		for l in p.libs:   # add plugin specific libs
			libs += ' %s' % Utils.quote_define_name(l[0])

		if p.name == 'geanylua':
			build_lua(bld, p, libs) # build additional lib for the lua plugin

		obj					        = bld.new_task_gen('cc', 'shlib')
		obj.source			        = p.sources
		obj.includes				= p.includes
		obj.env['shlib_PATTERN']    = '%s.so'
		obj.target			        = p.name
		obj.uselib		            = libs
		obj.install_path			= '${GEANY_LIBDIR}/geany'
		# if we are compiling more than one plugin, allow some of to fail
		#~ Runner.Parallel.error_handler = error_handler

		if os.path.exists(os.path.join(p.name, 'po')):
			obj		    = bld.new_task_gen('intltool_po')
			obj.podir   = os.path.join(p.name, 'po')
			obj.appname = p.name



def build_lua(bld, p, libs):
	lua_sources = [ 'geanylua/glspi_init.c', 'geanylua/glspi_app.c', 'geanylua/glspi_dlg.c',
					'geanylua/glspi_doc.c', 'geanylua/glspi_kfile.c', 'geanylua/glspi_run.c',
					'geanylua/glspi_sci.c', 'geanylua/gsdlg_lua.c' ]

	obj					        = bld.new_task_gen('cc', 'shlib')
	obj.source			        = lua_sources
	obj.includes				= p.includes
	obj.env['shlib_PATTERN']    = '%s.so'
	obj.target			        = 'libgeanylua'
	obj.uselib		            = libs
	obj.install_path			= '${DATADIR}/geany/plugins/geanylua'



def init():
	if Options.options.list_plugins:
		Utils.pprint('GREEN', \
			'The following targets can be chosen with the --enable-plugins option:')
		for p in plugins:
			print p.name,
		Utils.pprint('GREEN', \
	'\nTo compile only "%(1)s" and "%(2)s", use "./waf configure --enable-plugins=%(1)s,%(2)s".' % \
			{ '1' : plugins[0].name, '2' : plugins[1].name } )
		sys.exit(0)


def shutdown():
	if Options.options.update_po:
		# the following code is based on code from midori's WAF script, thanks
		for p in plugins:
			if not p.name in Build.bld.env['enabled_plugins']:
				continue;
			dir = os.path.join(p.name, 'po')
			try:
				os.chdir(dir)
				try:
					try:
						size_old = os.stat(p.name + '.pot').st_size
					except:
						size_old = 0
					subprocess.call(['intltool-update', '--pot'])
					size_new = os.stat(p.name + '.pot').st_size
					if size_new != size_old:
						Utils.pprint('CYAN', 'Updated POT file for %s.' % p.name)
						launch('intltool-update -r', 'Updating translations for %s' % p.name, 'CYAN')
					else:
						Utils.pprint('CYAN', 'POT file is up to date for %s.' % p.name)
				except:
					Utils.pprint('RED', 'Failed to generate pot file for %s.' % p.name)
				os.chdir(os.path.join('..', '..'))
			except:
				pass



# Simple function to execute a command and print its exit status
def launch(command, status, success_color='GREEN'):
	ret = 0
	Utils.pprint(success_color, status)
	try:
		ret = subprocess.call(command.split())
	except:
		ret = 1

	if ret != 0:
		Utils.pprint('RED', status + ' failed')

	return ret


def print_message(conf, msg, result, color = 'GREEN'):
	conf.check_message_1(msg)
	conf.check_message_2(result, color)

