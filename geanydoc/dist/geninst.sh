#!/bin/sh
#
# Generate PKGBUILD, spec and ebuild files
#

PKGNAME=`cat ../configure.in | awk '/AC_INIT/ {split($0, a, "["); v = a[2]; sub("\\\], ", "", v);print v;}'`
VERSION=`cat ../configure.in | awk '/AC_INIT/ {split($0, a, "["); v = a[3]; sub("\\\], ", "", v);print v;}'`

MD5SUM=`md5sum ../${PKGNAME}-${VERSION}.tar.gz | awk '// {print $1}'`

if [ -f PKGBUILD.in ]; then
    awk -v md5=$MD5SUM -v ver=$VERSION '/VERSION/ {gsub("VERSION",ver,$0);} // {print} /^source=/ {printf("md5sums=(%s)\n", md5)}' < PKGBUILD.in > PKGBUILD
fi

if [ -f ebuild.in ]; then
    cp ebuild.in  ${PKGNAME}-${VERSION}.ebuild
fi

if [ -f setup.nsi.in ]; then
#    cat setup.nsi.in | sed 's/VERSION/$VERSION/g' | sed 's/PKGNAME/${PKGNAME}' > ${PKGNAME}.nsi    
    cat setup.nsi.in | sed s/VERSION/${VERSION}/g | sed s/PKGNAME/${PKGNAME}/g   > ${PKGNAME}.nsi    
fi
