#!/bin/bash
MPIHOME=/usr/lib64/openmpi
MPIRUN=${MPIHOME}/bin/mpirun
BASEDIR=/home/gkenyon/workspace/SynthCog3
COMMAND="${BASEDIR}/Debug/SynthCog3 -p ${BASEDIR}/input/CatVsNoCatDog/Test/nocatdog2/canny3way/CatVsNoCatDog_Test_nocatdog2_canny3way.params -rows 4 -columns 6"
LOGFILE=/nh/compneuro/Data/ImageNet/PetaVision/CatVsNoCatDog/Test/activity/nocatdog2/canny3way/CatVsNoCatDog_Test_nocatdog2_canny3way.log
time ${MPIRUN} --bynode --hostfile ~/.mpi_anterior_hosts_second_half -np 24 ${COMMAND} 1> ${LOGFILE}
