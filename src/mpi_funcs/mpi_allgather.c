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

static void MPI_Allgather_prolog(CONST void* sendbuf MAYBE_UNUSED,
                                 int sendcount MAYBE_UNUSED,
				 MPI_Datatype sendtype MAYBE_UNUSED,
                                 void* recvbuf MAYBE_UNUSED,
                                 int  recvcount MAYBE_UNUSED,
                                 MPI_Datatype recvtype MAYBE_UNUSED,
				 MPI_Comm comm MAYBE_UNUSED) {

}

static int MPI_Allgather_core(CONST void* sendbuf, int sendcount,
                              MPI_Datatype sendtype, void* recvbuf,
                              int recvcount, MPI_Datatype recvtype,
                              MPI_Comm comm) {
  return libMPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                          recvtype, comm);
}

static void MPI_Allgather_epilog(CONST void* sendbuf MAYBE_UNUSED,
                                 int sendcount MAYBE_UNUSED,
				 MPI_Datatype sendtype MAYBE_UNUSED,
                                 void* recvbuf MAYBE_UNUSED,
                                 int  recvcount MAYBE_UNUSED,
                                 MPI_Datatype recvtype MAYBE_UNUSED,
				 MPI_Comm comm MAYBE_UNUSED) {
}


int MPI_Allgather(CONST void* sendbuf, int sendcount, MPI_Datatype sendtype,
                  void* recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPI_Comm comm) {
  FUNCTION_ENTRY;

  MPI_Allgather_prolog(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                       recvtype, comm);
  int ret = MPI_Allgather_core(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                               recvtype, comm);
  MPI_Allgather_epilog(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                       recvtype, comm);
  FUNCTION_EXIT;
  return ret;
}

void mpif_allgather_(void* sbuf, int* scount, MPI_Fint* sd, void* rbuf,
                     int* rcount, MPI_Fint* rd, MPI_Fint* c, int* error) {
  FUNCTION_ENTRY_("mpi_allgather_");
  MPI_Datatype c_stype = MPI_Type_f2c(*sd);
  MPI_Datatype c_rtype = MPI_Type_f2c(*rd);
  MPI_Comm c_comm = MPI_Comm_f2c(*c);
  void* c_sbuf = CHECK_MPI_IN_PLACE(sbuf);
  void* c_rbuf = CHECK_MPI_IN_PLACE(rbuf);

  MPI_Allgather_prolog(c_sbuf, *scount, c_stype, c_rbuf, *rcount, c_rtype, c_comm);
  *error = MPI_Allgather_core(c_sbuf, *scount, c_stype, c_rbuf, *rcount, c_rtype,
                              c_comm);
  MPI_Allgather_epilog(c_sbuf, *scount, c_stype, c_rbuf, *rcount, c_rtype, c_comm);
  FUNCTION_EXIT_("mpi_allgather_");
}
