
















function die() {
    echo "$1" 1>&2
    exit 1
}


TARGET=
declare -a ARGS
declare -a REMOTEARGS
index=0
for arg in "$@"; do
    case $arg in
	_REMOTEBUILD_TARGET=*)
	    TARGET=`echo $arg | awk -F= '{print $2}'`
	    continue
	    ;;
	_REMOTEBUILD_MAKE=*)
	    MAKE=`echo $arg | awk -F= '{print $2}'`
	    continue
	    ;;
	REMOTEBUILD=*)

	    ;;
	SRCROOT=*)
	    continue
	    ;;
	OBJROOT=*)
	    continue
	    ;;
	SYMROOT=*)
	    continue
	    ;;
	DSTROOT=*)
	    continue
	    ;;
	CCHROOT=*)
	    continue
	    ;;
	RC_XBS=*)

	    arg="VERBOSE=YES"
	    continue
	    ;;
	VERBOSE=YES)
	    set -x
	    ;;
    esac
    ARGS[$index]="$arg"
    REMOTEARGS[$index]="\"$arg\""
    index=$(($index+1))
done


RSYNC_ARGS="-azvh"
ARGS[$index]="REMOTEBUILD="
REMOTEARGS[$index]="\"REMOTEBUILD=\""


SKIPREMOTE=0
case $TARGET in
    clean)
	SKIPREMOTE=1
	;;
    installsrc)
	SKIPREMOTE=1
	;;
    installopensource)
	SKIPREMOTE=1
	;;
    cscope)
	SKIPREMOTE=1
	;;
    tags)
	SKIPREMOTE=1
	;;
    help)
	SKIPREMOTE=1
	;;
esac

if [ $SKIPREMOTE -eq 1 ]; then
    exec "$MAKE" "$TARGET" "${ARGS[@]}"
fi

SRC="$(pwd -P)"
SRCNAME="$(basename $SRC)"


OBJROOT="${OBJROOT}"
SYMROOT="${SYMROOT}"
DSTROOT="${DSTROOT}"

if [ -z "${OBJROOT}" ]; then
    die "OBJROOT not set in environment"
fi
mkdir -p "${OBJROOT}" || die "Could not create ${OBJROOT}"

if [ -z "${SYMROOT}" ]; then
    die "SYMROOT not set in environment"
fi
mkdir -p "${SYMROOT}" || die "Could not create ${SYMROOT}"

if [ -z "${DSTROOT}" ]; then
    die "DSTROOT not set in environment"
fi
mkdir -p "${DSTROOT}" || die "Could not create ${DSTROOT}"

if [ "$REMOTEBUILD" = "$SPECIALREMOTEBUILD" ]; then
    :
else
    DOINSTALLSRC=0
    REMOTE_SRCREL="./"
    BUILDTOOLSDIR="$OBJROOT"
    REMOTE_BUILDTOOLSREL="./BUILD/obj"
    BUILDSCRIPTDIR="$OBJROOT"
    REMOTE_BUILDSCRIPTREL="./BUILD/obj"
    BUILDSCRIPTNAME="build.sh"
    if [ ! -d "${OBJROOT}/SETUP" ]; then
    RSYNC_DELETE_EXCLUDED="--delete-excluded"
    else
    RSYNC_DELETE_EXCLUDED=""
    fi
    if [ ! -e "${SYMROOT}/" ]; then
	RSYNC_DELETE_SYMROOT=1
    else
	RSYNC_DELETE_SYMROOT=0
    fi
    if [ ! -e "${DSTROOT}/" ]; then
	RSYNC_DELETE_DSTROOT=1
    else
	RSYNC_DELETE_DSTROOT=0
    fi
    TARBUILDDIRS=0
fi

echo "Caching build tools..." 1>&2
mkdir -p "${BUILDTOOLSDIR}" || die "Could not create BUILDTOOLSDIR"
$MAKE print_exports "${ARGS[@]}" XCRUN="${SRC}/tools/xcrun_cache.sh -c \"${BUILDTOOLSDIR}\"" >/dev/null || die "Could not cache build tools"


MAKE_SDKROOT=`"${SRC}/tools/xcrun_cache.sh" -u "${BUILDTOOLSDIR}" -sdk / -show-sdk-path`
"${SRC}/tools/xcrun_cache.sh" -c "${BUILDTOOLSDIR}" -sdk "${MAKE_SDKROOT}" -find make >/dev/null || die "Could not cache make"


mkdir -p "${BUILDSCRIPTDIR}" || die "Could not create BUILDSCRIPTDIR"
cat > "${BUILDSCRIPTDIR}/${BUILDSCRIPTNAME}" <<EOF

mkdir -p /private/tmp
mkdir -p /private/var/tmp
mkdir -p "\${TMPDIR}"
cd "${REMOTE_SRCREL}"
mkdir -p ./BUILD/obj
mkdir -p ./BUILD/sym
mkdir -p ./BUILD/dst
MAKE=\`\$PWD/tools/xcrun_cache.sh -u "\$PWD/${REMOTE_BUILDTOOLSREL}" -sdk / -find make\`
if [ -z "\${MAKE}" ]; then exit 1; fi
\${MAKE} ${TARGET} ${REMOTEARGS[@]} XCRUN="\$PWD/tools/xcrun_cache.sh -u \"\$PWD/${REMOTE_BUILDTOOLSREL}\""
ret=\$?
if [ \$ret -eq 0 ]; then
if [ ${TARBUILDDIRS} -eq 1 ]; then
tar jcf ./BUILD/obj.tar.bz2 --exclude=\*.o --exclude=\*.cpo --exclude=\*.d --exclude=\*.cpd --exclude=\*.non_lto --exclude=\*.ctf --exclude=conf -C ./BUILD/obj . || exit 1
tar jcf ./BUILD/sym.tar.bz2 -C ./BUILD/sym . || exit 1
tar jcf ./BUILD/dst.tar.bz2 -C ./BUILD/dst . || exit 1
fi
fi
exit \$ret
EOF
chmod a+x "${BUILDSCRIPTDIR}/${BUILDSCRIPTNAME}"



mkdir -p "${BUILDTOOLSDIR}/empty"

if [ "$REMOTEBUILD" = "$SPECIALREMOTEBUILD" ]; then
    :
else

    REMOTEBUILD="$REMOTEBUILD"
    REMOTEBUILDPATH="$REMOTEBUILDPATH"

    if [ -z "$REMOTEBUILDPATH" ]; then
	WHOAMI=`whoami`
	case "${REMOTEBUILD}" in
	    *@*)
		WHOAMI=`echo "${REMOTEBUILD}" | awk -F@ '{print $1}'`
		;;
	esac
	REMOTEBUILDPATH="/tmp/$WHOAMI"
    fi


    eval `stat -s "${SRC}"`

    REMOTEBUILDPATH="${REMOTEBUILDPATH}/$st_ino/${SRCNAME}/"
    echo "Remote path is ${REMOTEBUILD}:${REMOTEBUILDPATH}" 1>&2

    ssh $REMOTEBUILD "mkdir -p \"${REMOTEBUILDPATH}/BUILD/\"{obj,sym,dst}" || die "Could not make remote build directory"


    rsync $RSYNC_ARGS --delete --exclude=\*~ --exclude=.svn --exclude=.git --exclude=/BUILD . $REMOTEBUILD:"${REMOTEBUILDPATH}" || die "Could not rsync source tree"


    rsync $RSYNC_ARGS --delete $RSYNC_DELETE_EXCLUDED --include=/build.sh --include=/BuildTools --include=/BuildTools/\*\* --exclude=\* "${OBJROOT}/" $REMOTEBUILD:"${REMOTEBUILDPATH}/BUILD/obj/" || die "Could not rsync build tree"


    if [ "$RSYNC_DELETE_SYMROOT" -eq 1 ]; then
	rsync $RSYNC_ARGS --delete "${BUILDTOOLSDIR}/empty/" $REMOTEBUILD:"${REMOTEBUILDPATH}/BUILD/sym/" || die "Could not rsync delete SYMROOT"
    fi


    if [ "$RSYNC_DELETE_DSTROOT" -eq 1 ]; then
	rsync $RSYNC_ARGS --delete "${BUILDTOOLSDIR}/empty/" $REMOTEBUILD:"${REMOTEBUILDPATH}/BUILD/dst/" || die "Could not rsync delete DSTROOT"
    fi


    echo ssh $REMOTEBUILD "/bin/bash -c 'cd \"${REMOTEBUILDPATH}\" && ${REMOTE_BUILDSCRIPTREL}/${BUILDSCRIPTNAME}'" 1>&2
    ssh $REMOTEBUILD "/bin/bash -c 'cd \"${REMOTEBUILDPATH}\" && ${REMOTE_BUILDSCRIPTREL}/${BUILDSCRIPTNAME}'" || die "Could not complete remote build"


    echo "Copying results back..."
    rsync $RSYNC_ARGS --no-o --no-g --exclude=\*.o --exclude=\*.cpo --exclude=\*.d --exclude=\*.cpd --exclude=\*.non_lto --exclude=\*.ctf $REMOTEBUILD:"${REMOTEBUILDPATH}/BUILD/obj/" "${OBJROOT}/" || die "Could not rsync build results"
    rsync $RSYNC_ARGS --no-o --no-g $REMOTEBUILD:"${REMOTEBUILDPATH}/BUILD/sym/" "${SYMROOT}/" || die "Could not rsync build results"
    rsync $RSYNC_ARGS --no-o --no-g $REMOTEBUILD:"${REMOTEBUILDPATH}/BUILD/dst/" "${DSTROOT}/" || die "Could not rsync build results"

fi

exit 0
