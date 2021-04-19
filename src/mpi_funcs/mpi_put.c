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

static void MPI_Put_prolog(CONST void* origin_addr  MAYBE_UNUSED,
                           int origin_count  MAYBE_UNUSED,
                           MPI_Datatype origin_datatype MAYBE_UNUSED,
                           int target_rank MAYBE_UNUSED,
                           MPI_Aint target_disp  MAYBE_UNUSED,
                           int target_count MAYBE_UNUSED,
                           MPI_Datatype target_datatype MAYBE_UNUSED,
                           MPI_Win win MAYBE_UNUSED ) {

}

static int MPI_Put_core(CONST void* origin_addr,
			int origin_count,
                        MPI_Datatype origin_datatype,
			int target_rank,
                        MPI_Aint target_disp,
			int target_count,
                        MPI_Datatype target_datatype,
			MPI_Win win) {
  LOCK();
  /* warning. MPI_Put is blocking, so this may lead to a deadlock */
  int ret = libMPI_Put(origin_addr, origin_count, origin_datatype, target_rank,
                    target_disp, target_count, target_datatype, win);
  UNLOCK();
  return ret;
}


static void MPI_Put_epilog(CONST void* origin_addr  MAYBE_UNUSED,
                           int origin_count  MAYBE_UNUSED,
                           MPI_Datatype origin_datatype MAYBE_UNUSED,
                           int target_rank MAYBE_UNUSED,
                           MPI_Aint target_disp  MAYBE_UNUSED,
                           int target_count MAYBE_UNUSED,
                           MPI_Datatype target_datatype MAYBE_UNUSED,
                           MPI_Win win MAYBE_UNUSED ) {

}

int MPI_Put(CONST void* origin_addr,
	    int origin_count,
	    MPI_Datatype origin_datatype,
	    int target_rank,
	    MPI_Aint target_disp,
	    int target_count,
	    MPI_Datatype target_datatype,
	    MPI_Win win) {
  FUNCTION_ENTRY;
  MPI_Put_prolog(origin_addr, origin_count, origin_datatype, target_rank,
                 target_disp, target_count, target_datatype, win);
  int ret = MPI_Put_core(origin_addr, origin_count, origin_datatype,
                         target_rank, target_disp, target_count,
                         target_datatype, win);
  MPI_Put_epilog(origin_addr, origin_count, origin_datatype, target_rank,
                 target_disp, target_count, target_datatype, win);
  FUNCTION_EXIT;
  return ret;
}

void mpif_put_(void* o_addr,
	       int* o_count,
	       MPI_Fint* o_d,
	       int* t_rank,
	       MPI_Aint* t_disp,
	       int* t_count,
	       MPI_Fint* t_d,
	       MPI_Fint* win,
               int* error) {
  FUNCTION_ENTRY_("mpi_put_");
  MPI_Datatype c_otype = MPI_Type_f2c(*o_d);
  MPI_Datatype c_ttype = MPI_Type_f2c(*t_d);
  MPI_Win c_win = MPI_Win_f2c(*win);

  MPI_Put_prolog(o_addr, *o_count, c_otype, *t_rank, *t_disp, *t_count, c_ttype,
                 c_win);
  *error = MPI_Put_core(o_addr, *o_count, c_otype, *t_rank, *t_disp, *t_count,
                        c_ttype, c_win);
  MPI_Put_epilog(o_addr, *o_count, c_otype, *t_rank, *t_disp, *t_count, c_ttype,
                 c_win);
  FUNCTION_EXIT_("mpi_put_");
}
