/* -*- c-file-style: "GNU" -*- */
/*
 * Copyright (C) CNRS, INRIA, Universite Bordeaux 1, Telecom SudParis
 * See COPYING in top-level directory.
 */

#ifndef _REENTRANT
#define _REENTRANT
#endif

#include "mpii.h"

#include <dlfcn.h>
#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <unistd.h>

static void MPI_Probe_prolog(int source  MAYBE_UNUSED,
                             int tag  MAYBE_UNUSED,
                             MPI_Comm comm  MAYBE_UNUSED,
                             MPI_Status* status MAYBE_UNUSED ) {

}

static int MPI_Probe_core(int source,
			  int tag,
			  MPI_Comm comm,
                          MPI_Status* status) {
  if(should_lock) {
    /* MPI_Probe is blocking. So we should not call it while holding the lock.
     * Replace MPI_Probe with an active waiting
     */
    uint64_t count = 0;
    while(1) {
      int ret = 0;
      int flag;
      ret = MPI_Iprobe(source, tag, comm, &flag, status);
      if(flag)
	return ret;
      count++;

      if(count > 10) {      /* sleep a little bit to decrease contention */
	sched_yield();
      } else if (count > 100){
	/* sleep even more */
	usleep(10);
      }
    }
  } else {
    return libMPI_Probe(source, tag, comm, status);
  }
}


static void MPI_Probe_epilog(int source  MAYBE_UNUSED,
                             int tag  MAYBE_UNUSED,
                             MPI_Comm comm  MAYBE_UNUSED,
                             MPI_Status* status MAYBE_UNUSED ) {

}

int MPI_Probe(int source,
	      int tag,
	      MPI_Comm comm,
	      MPI_Status* status) {
  FUNCTION_ENTRY;
  MPI_Probe_prolog(source, tag, comm, status);
  int ret = MPI_Probe_core(source, tag, comm, status);
  MPI_Probe_epilog(source, tag, comm, status);
  FUNCTION_EXIT;
  return ret;
}

void mpif_probe_(int* source,
		 int* tag,
		 MPI_Fint* comm,
		 MPI_Status* status,
                 int* err) {
  FUNCTION_ENTRY_("mpi_probe_");
  MPI_Comm c_comm = MPI_Comm_f2c(*comm);
  MPI_Probe_prolog(*source, *tag, c_comm, status);
  *err = MPI_Probe_core(*source, *tag, c_comm, status);
  MPI_Probe_epilog(*source, *tag, c_comm, status);
  FUNCTION_EXIT_("mpi_probe_");
}
