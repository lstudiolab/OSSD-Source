
copyright='
/*
 * Copyright (c) 2000-2019 Apple Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 * 
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.1 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
 * http://www.apple.com/publicsource and read it before using this file.
 * 
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved
 * Copyright (c) 1992, 1993, 1994, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
'
SCRIPT_ID='@(#)vnode_if.sh	8.7 (Berkeley) 5/11/95'







if [ $# -ne 1 ] ; then
	echo 'usage: vnode_if.sh srcfile'
	exit 1
fi


src=$1


out_c=vnode_if.c
out_h=vnode_if.h



awk=${AWK:-awk}



isgawk=`$awk 'BEGIN { print toupper("true"); exit; }' 2>/dev/null`


if [ "$isgawk" = TRUE ] ; then

	toupper=
else

	toupper='
function toupper(str) {
	_toupper_cmd = "echo "str" |tr a-z A-Z"
	_toupper_cmd | getline _toupper_str;
	close(_toupper_cmd);
	return _toupper_str;
}'
fi













sed_prep='s:\*\([^\*/]\):\* \1:g
s/;/ /'
awk_parser='

/^#/	{ next; }

/^vop_/	{
	name=$1;
	argc=0;
	ubc=$3;
	next;
}

/^}/	{
	doit();
	next;
}

{
	argdir[argc] = $1; i=2;
	if ($2 == "WILLRELE") {
		willrele[argc] = 1;
		i++;
	} else
		willrele[argc] = 0;
	argtype[argc] = $i; i++;
	while (i < NF) {
		argtype[argc] = argtype[argc]" "$i;
		i++;
	}
	argname[argc] = $i;
	argc++;
	next;
}
'

# This is put after the copyright on each generated file.
warning="
/*
 * Warning: This file is generated automatically.
 * (Modifications made here may easily be lost!)
 *
 * Created by the script:
 *	${SCRIPT_ID}
 */
"


space_elim='s:\([^/]\*\) :\1:g'




echo "$0: Creating $out_h" 1>&2
exec > $out_h


echo "$copyright"
echo "$warning"
echo '







extern struct vnodeop_desc vop_default_desc;
'



sed -e "$sed_prep" $src | $awk "$toupper"'
function doit() {

	printf("\nstruct %s_args {\n", name);
	printf("\tstruct vnodeop_desc * a_desc;\n");
	for (i=0; i<argc; i++) {
		printf("\t%s a_%s;\n", argtype[i], argname[i]);
	}
	printf("};\n");
	printf("extern struct vnodeop_desc %s_desc;\n", name);

	printf("#define %s(", toupper(name));
	for (i=0; i<argc; i++) {
		printf("%s", argname[i]);
		if (i < (argc-1)) printf(", ");
	}
	printf(") _%s(", toupper(name));
	for (i=0; i<argc; i++) {
		printf("%s", argname[i]);
		if (i < (argc-1)) printf(", ");
	}
	printf(")\n");
	printf("static __inline int _%s(", toupper(name));
	for (i=0; i<argc; i++) {

		printf("%s _%s", argtype[i], argname[i]);
		if (i < (argc-1)) printf(", ");
	}
	printf(")\n");
	printf("{\n\tstruct %s_args a;\n", name);
	printf("\ta.a_desc = VDESC(%s);\n", name);
	for (i=0; i<argc; i++) {
		printf("\ta.a_%s = _%s;\n", argname[i], argname[i]);
	}
	if (toupper(ubc) == "UBC") {
		printf("\t{\n\t\t" \
			"int _err;\n\t\t"   \
			"_err = VCALL(_%s%s, VOFFSET(%s), &a);\n\t\t"    \
			"return (_err);\n\t}\n}\n",
			argname[0], argname[0], arg0special, name, argname[0]);
	} else {
		printf("\treturn (VCALL(_%s%s, VOFFSET(%s), &a));\n}\n",
			argname[0], arg0special, name);
	}
}
BEGIN	{
	arg0special="";
}
END	{
	printf("\n/* Special cases: */\n#include <sys/buf.h>\n#include <sys/vm.h>\n");
	argc=1;
	argtype[0]="struct buf *";
	argname[0]="bp";
	arg0special="->b_vp";
	name="vop_strategy";
	doit();
	name="VNOP_BWRITE";
	doit();
}
'"$awk_parser" | sed -e "$space_elim"


echo '
/* End of special cases. */







echo "$0: Creating $out_c" 1>&2
exec > $out_c


echo "$copyright"
echo "$warning"
echo '





struct vnodeop_desc vop_default_desc = {
	.vdesc_offset = 0,
	.vdesc_name = "default",
	.vdesc_flags = 0,
	.vdesc_vp_offsets = NULL,
	.vdesc_vpp_offset = VDESC_NO_OFFSET,
	.vdesc_cred_offset = VDESC_NO_OFFSET,
	.vdesc_proc_offset = VDESC_NO_OFFSET,
	.vdesc_componentname_offset = VDESC_NO_OFFSET,
	.vdesc_context_offset = VDESC_NO_OFFSET,
	.vdesc_transports = NULL,
};
'


sed -e "$sed_prep" $src | $awk '
function do_offset(typematch) {
	for (i=0; i<argc; i++) {
		if (argtype[i] == typematch) {
			printf("\tVOPARG_OFFSETOF(struct %s_args, a_%s),\n",
				name, argname[i]);
			return i;
		};
	};
	print "\tVDESC_NO_OFFSET,";
	return -1;
}

function doit() {

	printf("\nint %s_vp_offsets[] = {\n", name);
	for (i=0; i<argc; i++) {
		if (argtype[i] == "struct vnode *") {
			printf ("\tVOPARG_OFFSETOF(struct %s_args,a_%s),\n",
				name, argname[i]);
		}
	}
	print "\tVDESC_NO_OFFSET";
	print "};";

	printf("struct vnodeop_desc %s_desc = {\n", name);

	printf ("\t0,\n");

	printf ("\t\"%s\",\n", name);

	printf("\t0");
	vpnum = 0;
	for (i=0; i<argc; i++) {
		if (match(argtype[i], "struct vnode *") == 1) {
			if (willrele[i]) {
				if (argdir[i] ~ /OUT/) {
					printf(" | VDESC_VPP_WILLRELE");
				} else {
					printf(" | VDESC_VP%s_WILLRELE", vpnum);
				};
			}
		vpnum++;
		}
	}
	print ",";

	printf ("\t%s_vp_offsets,\n", name);

	do_offset("struct vnode **");

	do_offset("kauth_credential_t");

	do_offset("struct proc *");

	do_offset("struct componentname *");

	printf ("\tNULL,\n};\n");
}
END	{
	printf("\n/* Special cases: */\n");
	argc=1;
	argdir[0]="IN";
	argtype[0]="struct buf *";
	argname[0]="bp";
	willrele[0]=0;
	name="vop_strategy";
	doit();
	name="VNOP_BWRITE";
	doit();
}
'"$awk_parser" | sed -e "$space_elim"


echo '
/* End of special cases. */'



echo '
struct vnodeop_desc *vfs_op_descs[] = {
	&vop_default_desc,	/* MUST BE FIRST */
	&vop_strategy_desc,	/* XXX: SPECIAL CASE */
	&VNOP_BWRITE_desc,	/* XXX: SPECIAL CASE */
'


sed -e "$sed_prep" $src | $awk '
function doit() {
	printf("\t&%s_desc,\n", name);
}
'"$awk_parser"


echo '	NULL
};
'

exit 0




