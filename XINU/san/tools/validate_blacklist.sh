




IFS=$'\n'

blacklist_files=`sed -n -e '

	/\*/ d


	/^src/ {
		s/^src://
		p
	}
' $1`

ret=0

for f in $blacklist_files ; do
	if ! [[ -e $f ]] ; then
		echo "KASan: blacklisted file $f not found" >&2
		ret=1
	fi
done

exit $ret
