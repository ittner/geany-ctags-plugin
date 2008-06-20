#!/usr/bin/env python

# This dirty script can be used to generate geany project files for C/C++ project
#
# Usage:
# $ python prgen.py > .geanyprj
#

import os

header = """

[project]
name=insert_project_name_here
base_path=./
description=
run_cmd=
regenerate=false
type=None

[files]
"""

print header

# Walk dir
import os

c = 0
for root, dirs, files in os.walk("."):
	for name in files:
		if os.path.splitext(name)[1] in (".cpp", ".c", ".h", ".cxx"):
			print "file%d=%s" % (c, os.path.join(root, name))
			c = c + 1
