


# Copyright (c) 2008 Apple Inc. All rights reserved.




# as defined in and that are subject to the Apple Public Source License

















CONFIG_DIR=$1 
ARCH=$2
TARGET_FILE=$3

SUPPORTED_KPI_FILES=( BSDKernel Mach IOKit Libkern )
DEPENDENCY_NAMES=( com.apple.kpi.bsd com.apple.kpi.mach com.apple.kpi.iokit com.apple.kpi.libkern )

rm -f ${TARGET_FILE}

if [ ${ARCH} == "all" ]
then 
	echo "The following symbols are considered sustainable KPI on all architectures." >> ${TARGET_FILE}
	echo "Note that symbols may be exported by some (or all) architectures individually." >> ${TARGET_FILE}
else
	echo "The following symbols are considered sustainable KPI on architecture ${ARCH}." >> ${TARGET_FILE}
fi
echo  >> ${TARGET_FILE}

for (( i = 0 ; i < ${#SUPPORTED_KPI_FILES[@]} ; i++ ))
do
	echo "Exported by ${DEPENDENCY_NAMES[i]}:" >> ${TARGET_FILE}
	echo >> ${TARGET_FILE}
	if [  ${ARCH} == "all" ]
	then
		cat "${CONFIG_DIR}/${SUPPORTED_KPI_FILES[i]}.exports" | sed "s/^_//" | sed "s/:.*//" | sort >> ${TARGET_FILE}
	else
		cat "${CONFIG_DIR}/${SUPPORTED_KPI_FILES[i]}.${ARCH}.exports" | sed "s/^_//" | sed "s/:.*//" | sort  >> ${TARGET_FILE}
	fi
	echo >> ${TARGET_FILE}
done
