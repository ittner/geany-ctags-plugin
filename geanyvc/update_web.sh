#!/bin/sh
#

if [ "$1" = "" ]; then
USERNAME=yurand
else
USERNAME=$1
fi

PLUGIN=`pwd | xargs basename`

rst2html.py README > index.html
scp index.html ${USERNAME}@shell.sourceforge.net:/home/groups/g/ge/geany-plugins/htdocs/${PLUGIN}
rm index.html
