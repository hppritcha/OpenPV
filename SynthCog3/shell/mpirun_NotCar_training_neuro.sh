#!/bin/bash
MPIHOME=/usr/lib64/openmpi
MPIRUN=${MPIHOME}/bin/mpirun
BASEDIR=/home/gkenyon/workspace/SynthCog3
COMMAND="${BASEDIR}/Debug/SynthCog3 -p ${BASEDIR}/input/Heli/Training/NotCar/canny3way/Heli_Training_NotCar_canny3way.params -rows 3 -columns 4"
LOGFILE=/nh/compneuro/Data/repo/neovision-programs-petavision/Heli/Training/activity/NotCar/canny3way/Heli_Training_NotCar_canny3way.log
time ${MPIRUN} --byslot -np 12 ${COMMAND} 1> ${LOGFILE}
