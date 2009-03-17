#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# WAF build script for geany-plugins
#
# Copyright 2008-2009 Enrico Tröger <enrico(dot)troeger(at)uvena(dot)de>
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

Requires WAF 1.5.3 and Python 2.4 (or later).
"""


import Build, Options, Utils, preproc
import sys, os


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
	Plugin('addons',
		 [ 'addons/src/addons.c', 'addons/src/ao_doclist.c', 'addons/src/ao_openuri.c' ],
		 [ 'addons', 'addons/src' ],
		 '0.1'),
	Plugin('externdbg',
		 [ 'externdbg/src/dbg.c' ], # source files
		 [ 'externdbg', 'externdbg/src' ], # include dirs
		 '0.1'),
	Plugin('geanygdb',
		 map(lambda x: "geanygdb/src/" + x, ['gdb-io-break.c',
		   'gdb-io-envir.c', 'gdb-io-frame.c', 'gdb-io-read.c', 'gdb-io-run.c',
		   'gdb-io-stack.c', 'gdb-lex.c', 'gdb-ui-break.c', 'gdb-ui-envir.c',
		   'gdb-ui-frame.c',  'gdb-ui-locn.c', 'gdb-ui-main.c',
		   'geanydebug.c']), # source files
		 [ 'geanygdb', 'geanygdb/src' ], # include dirs
		 '0.1'),
	Plugin('geanysendmail',
		 [ 'geanysendmail/src/geanysendmail.c' ], # source files
		 [ 'geanysendmail', 'geanysendmail/src' ], # include dirs
		 '0.5dev'),
	Plugin('geanydoc',
		 [ 'geanydoc/src/config.c', 'geanydoc/src/geanydoc.c' ], # source files
		 [ 'geanydoc', 'geanydoc/src' ], # include dirs
		 '0.3'),
	Plugin('geanylatex',
		 [ 'geanylatex/src/latexencodings.c', 'geanylatex/src/geanylatex.c',
		   'geanylatex/src/letters.c', 'geanylatex/src/bibtex.c',
		   'geanylatex/src/bibtexlabels.c', 'geanylatex/src/reftex.c',
		   'geanylatex/src/latexutils.c', 'geanylatex/src/formatutils.c',
		   'geanylatex/src/formatpatterns.c',
		   'geanylatex/src/latexenvironments.c'],
		 [ 'geanylatex' ], # include dirs
		 '0.4dev'),
	Plugin('geanylua',
		 [ 'geanylua/geanylua.c' ], # the other source files are listed in build_lua()
		 [ 'geanylua' ], # include dirs
		 # maybe you need to modify the package name of Lua, try one of these: lua5.1 lua51 lua-5.1
		 '0.7.0', [ [ 'lua', '5.1', True ] ]),
	Plugin('geanyprj',
		 [ 'geanyprj/src/geanyprj.c', 'geanyprj/src/menu.c', 'geanyprj/src/project.c',
		   'geanyprj/src/sidebar.c', 'geanyprj/src/utils.c', 'geanyprj/src/xproject.c' ],
		 [ 'geanyprj', 'geanyprj/src' ], # include dirs
		 '0.5'),
	Plugin('geanyvc',
		 [ 'geanyvc/geanyvc.c', 'geanyvc/utils.c', 'geanyvc/externdiff.c',
		   'geanyvc/vc_git.c', 'geanyvc/vc_cvs.c', 'geanyvc/vc_svn.c',
		   'geanyvc/vc_svk.c', 'geanyvc/vc_bzr.c', 'geanyvc/vc_hg.c' ],
		 [ 'geanyvc' ], # include dirs
		 '0.6', [ [ 'gtkspell-2.0', '2.0', False ] ]),
	Plugin('spellcheck',
		 [ 'spellcheck/src/gui.c', 'spellcheck/src/scplugin.c', 'spellcheck/src/speller.c' ], # source files
		 [ 'spellcheck', 'spellcheck/src' ], # include dirs
		 '0.4', [ [ 'enchant', '1.3', True ] ]),
	Plugin('geanylipsum',
		 [ 'geanylipsum/src/geanylipsum.c' ], # source files
		 [ 'geanylipsum', 'geanylipsum/src' ], # include dirs
		 '0.2dev'),
	Plugin('geany-mini-script',
		 [ 'geany-mini-script/src/gms.c', 'geany-mini-script/src/gms_gui.c' ], # source files
		 [ 'geany-mini-script', 'geany-mini-script/src' ], # include dirs
		 '0.2'),
	Plugin('shiftcolumn',
		 [ 'shiftcolumn/src/shiftcolumn.c'],
		 [ 'shiftcolumn', 'shiftcolumn/src'],
		 '0.3')
]

makefile_template = '''#!/usr/bin/make -f
# Waf Makefile wrapper

all:
	@./waf build

update-po:
	@./waf --update-po

install:
	@if test -n "$(DESTDIR)"; then \\
		./waf install --destdir="$(DESTDIR)"; \\
	else \\
		./waf install; \\
	fi;

uninstall:
	@if test -n "$(DESTDIR)"; then \\
		./waf uninstall --destdir="$(DESTDIR)"; \\
	else \\
		./waf uninstall; \\
	fi;

clean:
	@./waf clean

.PHONY: clean uninstall install all
'''


preproc.go_absolute = True
preproc.standard_includes = []

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
				stdout = Utils.cmd_output(cmd='svn info --non-interactive', env={'LANG' : 'C'})
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

	def set_lib_dir():
		# use the libdir specified on command line
		if Options.options.libdir:
			conf.define('LIBDIR', Options.options.libdir, 1)
		else:
			# get Geany's libdir (this should be the default case for most users)
			libdir = conf.check_cfg(package='geany', args='--variable=libdir')
			if libdir:
				conf.define('LIBDIR', libdir.strip(), 1)
			else:
				conf.define('LIBDIR', conf.env['PREFIX'] + '/lib', 1)

	conf.check_tool('compiler_cc intltool')

	set_lib_dir()

	conf.check_cfg(package='gtk+-2.0', atleast_version='2.6.0', uselib_store='GTK',
		mandatory=True, args='--cflags --libs')
	conf.check_cfg(package='geany', atleast_version='0.16', mandatory=True, args='--cflags --libs')

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
				conf.check_cfg(package=l[0], uselib_store=uselib, atleast_version=l[1], args='--cflags --libs')
				if not conf.env['HAVE_%s' % uselib] == 1:
					if l[2]:
						enabled_plugins.remove(p.name)


	svn_rev = conf_get_svn_rev()
	conf.define('REVISION', svn_rev, 1)

	# write a config.h for each plugin
	for p in plugins:
		if p.name in enabled_plugins:
			if p.name == 'geanyvc' and conf.env['HAVE_GTKSPELL_2_0'] == 1:
				# hack for GeanyVC
				conf.define('USE_GTKSPELL', 1);
			conf.define('VERSION', p.version, 1)
			conf.define('PACKAGE', p.name, 1)
			conf.define('PREFIX', conf.env['PREFIX'], 1)
			conf.define('DOCDIR', '%s/doc/geany-plugins/%s' % (conf.env['DATADIR'], p.name), 1)
			if os.path.exists(os.path.join(p.name, 'po')):
				conf.define('GETTEXT_PACKAGE', p.name, 1)
				conf.define('ENABLE_NLS', 1)
			else:
				conf.undefine('GETTEXT_PACKAGE')
				conf.undefine('ENABLE_NLS')
			conf.write_config_header(os.path.join(p.name, 'config.h'))

	Utils.pprint('BLUE', 'Summary:')
	print_message(conf, 'Install Geany Plugins ' + VERSION + ' in', conf.env['PREFIX'])
	print_message(conf, 'Using GTK version', gtk_version)
	print_message(conf, 'Using Geany version', geany_version)
	if svn_rev != '-1':
		print_message(conf, 'Compiling Subversion revision', svn_rev)
		conf.env.append_value('CCFLAGS', '-g -O0 -DDEBUG'.split()) # -DGEANY_DISABLE_DEPRECATED')

	print_message(conf, 'Plugins to compile', ' '.join(enabled_plugins))

	conf.env.append_value('enabled_plugins', enabled_plugins)
	conf.env.append_value('CCFLAGS', '-DHAVE_CONFIG_H'.split())

	# write a simply Makefile
	f = open('Makefile', 'w')
	print >>f, makefile_template
	f.close


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

	opt.add_option('--enable-plugins', action='store', default='',
		help='plugins to be built [plugins in CSV format, e.g. "%(1)s,%(2)s"]' % \
		{ '1' : plugins[0].name, '2' : plugins[1].name }, dest='enable_plugins')
	opt.add_option('--skip-plugins', action='store', default='',
		help='plugins which should not be built, ignored when --enable-plugins is set, same format as --enable-plugins' % \
		{ '1' : plugins[0].name, '2' : plugins[1].name }, dest='skip_plugins')
	opt.add_option('--target-win32', action='store_true', default=False,
		help='Cross-compile for Win32', dest='target_win32')


def build(bld):
	def build_lua(bld, p, libs):
		lua_sources = [ 'geanylua/glspi_init.c', 'geanylua/glspi_app.c', 'geanylua/glspi_dlg.c',
						'geanylua/glspi_doc.c', 'geanylua/glspi_kfile.c', 'geanylua/glspi_run.c',
						'geanylua/glspi_sci.c', 'geanylua/gsdlg_lua.c' ]

		bld.new_task_gen(
			features		= 'cc cshlib',
			source			= lua_sources,
			includes		= p.includes,
			target			= 'libgeanylua',
			uselib			= libs,
			install_path	= '${DATADIR}/geany/plugins/geanylua'
		)

		# install docs
		bld.install_files('${DATADIR}/doc/geany-plugins/geanylua', 'geanylua/docs/*.html')
		# install examples (Waf doesn't support installing files recursively, yet)
		bld.install_files('${DATADIR}/geany/plugins/geanylua/examples/dialogs', 'geanylua/examples/dialogs/*.lua')
		bld.install_files('${DATADIR}/geany/plugins/geanylua/examples/edit', 'geanylua/examples/edit/*.lua')
		bld.install_files('${DATADIR}/geany/plugins/geanylua/examples/info', 'geanylua/examples/info/*.lua')
		bld.install_files('${DATADIR}/geany/plugins/geanylua/examples/scripting', 'geanylua/examples/scripting/*.lua')
		bld.install_files('${DATADIR}/geany/plugins/geanylua/examples/work', 'geanylua/examples/work/*.lua')


	def build_debug(bld, p, libs):
		bld.new_task_gen(
			features	= 'cc cprogram',
			source		= [ 'geanygdb/src/ttyhelper.c' ],
			includes	= p.includes,
			target		= 'geanygdb_ttyhelper',
			uselib		= libs
		)

	def install_docs(bld, pname, files):
		for file in files:
			if os.path.exists(os.path.join(p.name, file)):
				bld.install_files('${DATADIR}/doc/geany-plugins/%s' % pname, '%s/%s' % (pname, file))


	# Build the plugins
	if Options.options.target_win32:
		bld.env['shlib_CCFLAGS']    = '' # disable default -fPIC flag
		bld.env['shlib_PATTERN']    = '%s.dll'
	else:
		bld.env['shlib_PATTERN']    = '%s.so'

	for p in plugins:
		if not p.name in bld.env['enabled_plugins']:
			continue;

		libs = 'GTK GEANY' # common for all plugins
		for l in p.libs:   # add plugin specific libs
			libs += ' %s' % Utils.quote_define_name(l[0])

		if p.name == 'geanylua':
			build_lua(bld, p, libs) # build additional lib for the lua plugin

		if p.name == 'geanygdb':
			build_debug(bld, p, libs) # build additional binary for the debug plugin

		bld.new_task_gen(
			features		= 'cc cshlib',
			source			= p.sources,
			includes		= p.includes,
			target			= p.name,
			uselib			= libs,
			install_path	= '${LIBDIR}/geany'
		)

		if os.path.exists(os.path.join(p.name, 'po')):
			bld.new_task_gen(
				features	= 'intltool_po',
				podir		= os.path.join(p.name, 'po'),
				appname		= p.name
			)
		install_docs(bld, p.name, 'AUTHORS ChangeLog COPYING NEWS README THANKS TODO'.split())


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
					Utils.exec_command('intltool-update --pot -g %s' % p.name)
					size_new = os.stat(p.name + '.pot').st_size
					if size_new != size_old:
						Utils.pprint('CYAN', 'Updated POT file for %s.' % p.name)
						launch('intltool-update -r -g %s' % p.name,
							'Updating translations for %s' % p.name, 'CYAN')
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
		ret = Utils.exec_command(command)
	except OSError, e:
		ret = 1
		print str(e), ":", command
	except:
		ret = 1

	if ret != 0:
		Utils.pprint('RED', status + ' failed')

	return ret


def print_message(conf, msg, result, color = 'GREEN'):
	conf.check_message_1(msg)
	conf.check_message_2(result, color)

