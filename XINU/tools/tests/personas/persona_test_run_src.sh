














PERSONA_MGR="${PWD}/persona_mgr"
PERSONA_SPAWN="${PWD}/persona_spawn"
PERSONA_SPAWN_UNENTITLED="${PWD}/persona_spawn_unentitled"

TEST_DEFAULT_PERSONA=0

if [ ! -d "$TMPDIR" ]; then
	echo "Couldn't find temp directory '$TMPDIR': check permissions/environment?"
	exit 255
fi

if [ ! -e "${PERSONA_MGR}" ] ||  [ ! -x "${PERSONA_MGR}" ]; then
	echo "Can't find '${PERSONA_MGR}': skipping test"
	exit 0
fi
if [ ! -e "${PERSONA_SPAWN}" ] || [ ! -x "${PERSONA_SPAWN}" ]; then
	echo "Can't find '${PERSONA_SPAWN}': skipping test"
	exit 0
fi

function check_for_persona_support() {
	local errno=0
	${PERSONA_MGR} support || errno=$?
	if [ $errno -eq 78 ]; then
		echo "Persona subsystem is not supported - skipping tests"
		exit 0
	fi
	return 0
}
check_for_persona_support








function bail() {
	local msg="$1"
	local line=$2
	if [ -z "$line" ]; then
		line=${BASH_LINENO[0]}
	fi
	echo "[$line] ERROR: $msg" 1>&2
	exit $((1 + $line % 254))
}






function check_return() {
	local err=$?
	local msg=$1
	local line=$2
	if [ -z "$line" ]; then
		line=${BASH_LINENO[0]}
	fi
	echo "CHECK: $msg"
	if [ $err -ne 0 ]; then
		bail "e=$err: $msg" $line
	fi

	return 0
}







function expect_failure() {
	local err=$?
	local msg=$1
	local line=$2
	if [ -z "$line" ]; then
		line=${BASH_LINENO[0]}
	fi
	if [ $err -eq 0 ]; then
		bail "found success, expected failure: $msg" $line
	fi

	echo "EXPECT: failure: $msg"
	return 0
}





function test_num() {
	local type=$1
	local num=$2
	local line=$3
	if [ -z "$line" ]; then
		line=${BASH_LINENO[0]}
	fi
	if [ -z "$num" ]; then
		bail "invalid (NULL) $type" $line
	fi
	[ "$num" -eq "$num" ] 2>/dev/null
	if [ $? -ne 0 ]; then
		bail "invalid $type: $num" $line
	fi

	return 0
}


_ID=-1
_TYPE="invalid"
_LOGIN=""
_UID=-1
_GID=-1
_NGROUPS=-1
_GROUPS=""





function get_persona_info() {
	local pna_id=${1:-1}
	local pna_login=${2:- }
	local line=$3
	if [ -z "$line" ]; then
		line=${BASH_LINENO[0]}
	fi

	local largs="-u ${pna_id}"
	if [ "${pna_login}" != " " ]; then
		largs+=" -l ${pna_login}"
	fi

	_ID=-1
	_TYPE=-1
	_LOGIN=""
	_UID=-1
	_GID=-1
	_NGROUPS=-1
	_GROUPS=()

	local file="${TMPDIR}/plookup"

	${PERSONA_MGR} lookup ${largs} > "${file}"
	check_return "persona lookup of: ${largs}" $line

	_ID=$(cat "${file}" | grep "+id: " | head -1 | sed 's/.*+id:[ ]*\([0-9][0-9]*\).*/\1/')
	test_num "Persona ID lookup:${largs}" "$_ID"

	local type=$(cat "${file}" | grep "+type: " | head -1 | sed 's/.*+type:[ ]*\([0-9][0-9]*\).*/\1/')
	test_num "+type lookup:${largs}" "$type"



	if [ $type -eq 1 ]; then
		_TYPE=guest
	elif [ $type -eq 2 ]; then
		_TYPE=managed
	elif [ $type -eq 3 ]; then
		_TYPE=priv
	elif [ $type -eq 4 ]; then
		_TYPE=system
	else
		_TYPE=invalid
	fi

	_LOGIN=$(cat "${file}" | grep "+login: " | head -1 | sed 's/.*+login:[ ]*"\([^"]*\)".*/\1/')
	if [ -z "$_LOGIN" ]; then
		bail "invalid login for pna_id:$_ID: '$_LOGIN'" $line
	fi


	_UID=$_ID

	_GID=$(cat "${file}" | grep "+gid: " | head -1 | sed 's/.*+gid:[ ]*\([0-9][0-9]*\).*/\1/')
	test_num "GID lookup:${largs}" "$_GID"

	_NGROUPS=$(cat "${file}" | grep "ngroups: " | head -1 | sed 's/.*ngroups:[ ]*\([0-9][0-9]*\)[ ][ ]*{.*}.*/\1/')
	test_num "NGROUPS lookup:${largs}" "$_NGROUPS"

	_GROUPS=( $(cat "${file}" | grep "ngroups: " | head -1 | sed 's/.*ngroups:[ ]*[0-9][0-9]*[ ][ ]*{[ ]*\([^ ].*\)[ ][ ]*}.*/\1/') )
	if [ $_NGROUPS -gt 0 ]; then
		if [ -z "${_GROUPS}" ]; then
			bail "lookup:${largs}: missing $_NGROUPS groups" $line
		fi
		if [ ${#_GROUPS[@]} -ne $_NGROUPS ]; then
			bail "lookup:${largs} wrong number of groups ${#_GROUPS[@]} != $_NGROUPS" $line
		fi
	fi
}







function validate_child_info() {
	local file=$1
	local pna_id=$2
	local uid=${3:--1}
	local gid=${4:--1}
	local groups=${5:- }
	local line=$6
	if [ -z "$line" ]; then
		line=${BASH_LINENO[0]}
	fi
	local l=( )


	local cpid="$(cat "$file" | grep "Child: PID:" | sed 's/.*Child: PID:\([0-9][0-9]*\).*/\1/')"
	test_num "Child PID" "$cpid" $line


	l=( $(cat "$file" | grep "Child: Persona:" | sed 's/.*Child: Persona: \([0-9][0-9]*\) (err:\([0-9][0-9]*\))/\1 \2/') )
	if [ ${#l[@]} -ne 2 ]; then
		bail "Invalid Child[$cpid] Persona line" $line
	fi
	test_num "Child Persona ID" "${l[0]}" $line
	test_num "kpersona_info retval" "${l[1]}" $line

	if [ ${l[0]} -ne $pna_id ]; then
		bail "Child[$cpid] persona:${l[0]} != specified persona:$pna_id" $line
	fi


	l=( $(cat "$file" | grep "Child: UID:" | sed 's/.*UID:\([0-9][0-9]*\), GID:\([0-9][0-9]*\).*/\1 \2/') )
	if [ ${#l[@]} -ne 2 ]; then
		bail "Invalid Child[$cpid] UID/GID output" $line
	fi
	if [ $uid -ge 0 ]; then
		if [ $uid -ne ${l[0]} ]; then
			bail "Child[$cpid] UID:${l[0]} != specified UID:$uid" $line
		fi
	fi
	if [ $gid -ge 0 ]; then
		if [ $gid -ne ${l[1]} ]; then
			bail "Child[$cpid] GID:${l[1]} != specified GID:$gid" $line
		fi
	fi



	return 0
}








function spawn_child() {
	local pna_id=$1
	local uid=${2:--1}
	local gid=${3:--1}
	local groups=${4:- }
	local line=$5
	if [ -z "$line" ]; then
		line=${BASH_LINENO[0]}
	fi

	local file="child.${pna_id}"
	local spawn_args="-I $pna_id"
	if [ $uid -ge 0 ]; then
		spawn_args+=" -u $uid"
		file+=".u$uid"
	fi
	if [ $gid -ge 0 ]; then
		spawn_args+=" -g $gid"
		file+=".g$gid"
	fi
	if [ "$groups" != " " ]; then
		spawn_args+=" -G $groups"
		file+="._groups"
	fi

	echo "SPAWN: $file"
	${PERSONA_SPAWN} -v $spawn_args ${PERSONA_SPAWN} child -v -E > "${TMPDIR}/$file"
	check_return "child info: $file" $line




	get_persona_info ${pna_id} " " $line
	if [ $uid -lt 0 ]; then
		uid=$_UID
	fi
	if [ $gid -lt 0 ]; then
		gid=$_GID
	fi
	if [ "$groups" == " " ]; then

		local _g="${_GROUPS[@]}"
		groups="${_g// /,}"
	fi

	validate_child_info "${TMPDIR}/$file" "$pna_id" "$uid" "$gid" "$groups" $line



	if [ $uid -eq 0 ]; then
		${PERSONA_SPAWN} -v $spawn_args ${PERSONA_SPAWN_UNENTITLED} child -v -E -R spawn -v $spawn_args -I ${TEST_DEFAULT_PERSONA} /bin/echo "This is running in the system persona"
		expect_failure "Spawned child that re-execs into non-default persona" $line
	fi
	return 0
}






function get_created_id() {
	local file=$1
	local o=$(cat "$file" | grep "Created persona" | sed 's/.*Created persona \([0-9][0-9]*\):/\1/')
	echo $o
	return 0
}







function create_persona() {
	local name=${1}
	local type=${2}
	local pna_id=${3:--1}
	local gid=${4:--1}
	local groups=${5:- }
	local line=$6
	if [ -z "$line" ]; then
		line=${BASH_LINENO[0]}
	fi

	if [ -z "$name" -o -z "$type" ]; then
		bail "Invalid arguments to create_persona '$name' '$type'" $line
	fi

	local file="persona.at${line}"

	local spawn_args="-v -l $name -i $pna_id"
	if [ $pna_id -eq -1 ]; then
		file+=".auto"
	else
		file+=".${pna_id}"
	fi

	spawn_args+=" -t $type"
	file+=".$type"

	if [ $gid -ge 0 ]; then
		spawn_args+=" -g $gid"
		file+=".g$gid"
	fi
	if [ "$groups" != " " ]; then
		spawn_args+=" -G $groups"
		file+="._groups"
	fi

	echo "CREATE: $file"
	${PERSONA_MGR} create ${spawn_args} > "${TMPDIR}/${file}"
	check_return "persona creation: ${file}" $line

	cat "${TMPDIR}/${file}"


	_ID=`get_created_id "${TMPDIR}/${file}"`
	test_num "persona_id for $file" "$_ID" $line
	if [ ${pna_id} -gt 0 ]; then
		if [ $_ID -ne ${pna_id} ]; then
			bail "Created persona doesn't have expected ID $_ID != ${pna_id}" $line
		fi
	fi



	echo "VALIDATE: ${file}"
	get_persona_info ${pna_id} "$name" $line
	if [ "$name" != "$_LOGIN" ]; then
		bail "${file}: unexpected login '$_LOGIN' != '$name'" $line
	fi
	if [ "$type" != "$_TYPE" ]; then
		bail "${file}: unexpected type '$_TYPE' != '$type'" $line
	fi
	if [ ${pna_id} -gt 0 ]; then
		if [ ${pna_id} -ne $_ID ]; then
			bail "${file}: unexpected ID '$_ID' != '${pna_id}'" $line
		fi
	fi
	if [ $gid -ge 0 ]; then
		if [ $gid -ne $_GID ]; then
			bail "${file}: unexpected GID '$_GID' != '$gid'" $line
		fi
	fi
	if [ "$groups" != " " ]; then
		local _g="${_GROUPS[@]}"
		if [ "${_g// /,}" != "$groups" ]; then
			bail "${file}: unexpected groups '${_g// /,}' != '$groups'" $line
		fi
	fi

	return 0
}





function destroy_persona() {
	local pna_id=$1
	local line=$2
	if [ -z "$line" ]; then
		line=${BASH_LINENO[0]}
	fi

	echo "DESTROY: ${pna_id}"
	${PERSONA_MGR} destroy -v -i ${pna_id}
	check_return "destruction of ${pna_id}" $line
}






echo "Running persona tests [$LINENO] ($TMPDIR)"





create_persona "test_default_persona" "guest" 9999
TEST_DEFAULT_PERSONA=$_ID


create_persona "test0_1" "guest" 1001
P0ID=$_ID

spawn_child $P0ID
spawn_child $P0ID 1100
spawn_child $P0ID 0
spawn_child $P0ID -1 1101
spawn_child $P0ID 1100 1101
spawn_child $P0ID 1100 1101 1000,2000,3000
spawn_child $P0ID 1100 -1 1000,2000,3000
spawn_child $P0ID -1 -1 1000,2000,3000
destroy_persona $P0ID


create_persona "test0_2" "guest" 1002 2000
P0ID=$_ID
spawn_child $P0ID
spawn_child $P0ID 1100
spawn_child $P0ID 0
spawn_child $P0ID -1 1101
spawn_child $P0ID 1100 1101
spawn_child $P0ID 1100 1101 1000,2000,3000
spawn_child $P0ID 1100 -1 1000,2000,3000
spawn_child $P0ID -1 -1 1000,2000,3000
destroy_persona $P0ID


create_persona "test0_3" "guest" 1003 2000 2000,3000,4000
P0ID=$_ID
spawn_child $P0ID
spawn_child $P0ID 1100
spawn_child $P0ID 0
spawn_child $P0ID -1 1101
spawn_child $P0ID 1100 1101
spawn_child $P0ID 1100 1101 1111,2222,3333
spawn_child $P0ID 1100 -1 1111,2222,3333
spawn_child $P0ID -1 -1 1111,2222,3333
destroy_persona $P0ID







create_persona "test1_1" "guest"
P1ID=$_ID
create_persona "test1_2" "managed"
P2ID=$_ID
create_persona "test1_3" "priv"
P3ID=$_ID
create_persona "test1_4" "guest"
P4ID=$_ID

D1=$(($P2ID - $P1ID))
D2=$(($P3ID - $P2ID))
D3=$(($P4ID - $P3ID))
if [ $D1 -ne $D2 -o $D1 -ne $D3 -o $D2 -ne $D3 ]; then
	bail "inconsistent automatic Persona ID increment: $D1,$D2,$D3 ($P1ID,$P2ID,$P3ID,$P4ID)"
fi


${PERSONA_MGR} create -v -l test1_1 -t guest -i -1 && expect_failure "re-create same name:test1_1 type:guest"
${PERSONA_MGR} create -v -l test1_1 -t managed -i -1 && expect_failure "re-create same name:test1_1 type:managed"
${PERSONA_MGR} create -v -l test1_1_new -t managed -i $P1ID && expect_failure "re-create $P1ID with new name:test1_1_new type:managed"








P5ID_REQ=$(($P4ID + $D2))
create_persona "test2_1" "guest" ${P5ID_REQ}
P5ID=$_ID
if [ ! $P5ID -eq ${P5ID_REQ} ]; then
	bail "test2_1: ${P5ID_REQ} != $P5ID"
fi



create_persona "test2_2" "guest"
P6ID=$_ID
if [ $P6ID -eq $P5ID ]; then
	bail "created duplicate persona IDs: $P6ID == $P5ID"
fi





destroy_persona $P1ID
destroy_persona $P2ID
destroy_persona $P3ID
destroy_persona $P4ID
destroy_persona $P5ID
destroy_persona $P6ID



${PERSONA_MGR} destroy -v -i $P1ID && expect_failure "re-destroy (1/2) $P1ID"
${PERSONA_MGR} destroy -v -i $P1ID && expect_failure "re-destroy (2/2) $P1ID"
${PERSONA_MGR} destroy -v -i $P2ID && expect_failure "re-destroy $P2ID"
${PERSONA_MGR} destroy -v -i $P3ID && expect_failure "re-destroy $P3ID"
${PERSONA_MGR} destroy -v -i $P4ID && expect_failure "re-destroy $P4ID"
${PERSONA_MGR} destroy -v -i $P5ID && expect_failure "re-destroy $P5ID"
${PERSONA_MGR} destroy -v -i $P6ID && expect_failure "re-destroy $P6ID"

destroy_persona ${TEST_DEFAULT_PERSONA}


rm -rf "${TMPDIR}"

echo ""
echo "${0##/}: SUCCESS"
exit 0
