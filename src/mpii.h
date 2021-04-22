#pragma once
#define _GNU_SOURCE
#include <dlfcn.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include "mpii_macros.h"
#include "mpii_config.h"
#include <pthread.h>

/* should we protect MPI from concurrent accesses ? */
extern int should_lock;
/* mutex used for protecting MPI from concurren calls */
extern pthread_mutex_t mpi_lock;

/* prevent the library from processing recursive MPI calls */
extern __thread int recursion_shield;

#define LOCK() do {				\
    if(should_lock) {				\
      pthread_mutex_lock(&mpi_lock);		\
    }						\
  } while(0)

#define UNLOCK() do {					\
    if(should_lock) pthread_mutex_unlock(&mpi_lock);	\
  } while(0)

struct ezt_instrumented_function {
  char function_name[1024];
  void* callback;
  int event_id;
};

extern struct ezt_instrumented_function hijack_list[];
#define INSTRUMENTED_FUNCTIONS hijack_list

struct mpii_info {
  int rank;
  int size;
  int mpi_any_source;
  int mpi_any_tag;
  int mpi_request_null;
  int mpi_proc_null;
  int mpi_comm_world;
  int mpi_comm_self;

  struct mpii_settings settings;
};
/* information on the local process */
extern struct mpii_info mpii_infos;


/* number of pending mpi calls. If MPI is not thread safe, this should
   always be 0 or 1 */
extern _Atomic int current_mpi_calls;
/* which thread is currently calling MPI ? */
extern volatile pthread_t current_mpi_call_thread;
/* which MPI function is currently being called ? */
extern volatile const char* current_mpi_call_function;

/* When entering an MPI function, check if another thread is currently
   using MPI */
#define CHECK_CONCURRENCY_ENTER_MPI(fname) do {				\
    if(mpii_infos.settings.check_concurrency != 0) {			\
      int nb_calls = ++current_mpi_calls;				\
      if( nb_calls != 1) {						\
	MPII_PRINTF(0, "Warning: thread %lx calls %s while thread %lx calls %s! %d\n", \
		    pthread_self(), fname,				\
		    current_mpi_call_thread, current_mpi_call_function, nb_calls); \
	if(mpii_infos.settings.abort_on_concurrency_check_failure) abort(); \
      }									\
      current_mpi_call_thread = pthread_self();				\
      current_mpi_call_function = fname;				\
    }									\
  } while(0)

/* When leaving an MPI function, check if another thread is currently
   using MPI */
#define CHECK_CONCURRENCY_LEAVE_MPI(fname) do {				\
    if(mpii_infos.settings.check_concurrency != 0) {			\
      int nb_calls = --current_mpi_calls;				\
      if( nb_calls != 0) {						\
	MPII_PRINTF(0, "Warning: thread %lx leaves %s while thread %lx is in %s! %d\n", \
		    pthread_self(), fname,				\
		    current_mpi_call_thread, current_mpi_call_function, nb_calls); \
	if(mpii_infos.settings.abort_on_concurrency_check_failure) abort(); \
      }									\
      current_mpi_call_thread = 0;					\
      current_mpi_call_function = NULL;					\
    }									\
  } while(0)

/* called when entering an MPI function */
#define FUNCTION_ENTRY_(fname) do {					\
    if(recursion_shield++ == 0) {					\
      CHECK_CONCURRENCY_ENTER_MPI(fname);				\
      MPII_PRINTF(2, "[%d/%d]\tEntering %s\n", mpii_infos.rank, mpii_infos.size, fname); \
    }									\
  } while(0)

/* called when leaving an MPI function */
#define FUNCTION_EXIT_(fname)  do {					\
    if(--recursion_shield == 0) {					\
      CHECK_CONCURRENCY_LEAVE_MPI(fname);				\
      MPII_PRINTF(2, "[%d/%d]\tLeaving %s\n", mpii_infos.rank, mpii_infos.size, fname);	\
    }									\
  } while(0)

#define MPII_PRINTF(_debug_level_, ...)			\
    {							\
      if (mpii_infos.settings.verbose >= _debug_level_) \
	fprintf(stderr, __VA_ARGS__);			\
    }

#define FUNCTION_ENTRY FUNCTION_ENTRY_(__func__);
#define FUNCTION_EXIT  FUNCTION_EXIT_(__func__);

/* maximum number of items to be allocated statically
 * if the application need more than this, a dynamic array
 * is allocated using malloc()
 */
#define MAX_REQS 128

/* allocate a number of elements using a static array if possible
 * if not possible (ie. count>MAX_REQS) use a dynamic array (ie. malloc)
 */
#define ALLOCATE_ITEMS(type, count, static_var, dyn_var)	\
  type static_var[MAX_REQS];					\
  type* dyn_var = static_var;					\
  if ((count) > MAX_REQS)					\
    dyn_var = (type*)malloc(sizeof(type) * (count))

/* Free an array created by ALLOCATE_ITEMS */
#define FREE_ITEMS(count, dyn_var)		\
  if ((count) > MAX_REQS)			\
    free(dyn_var)

/* pointers to actual MPI functions (C version)  */
extern int (*libMPI_Init)(int*, char***);
extern int (*libMPI_Init_thread)(int*, char***, int, int*);
extern int (*libMPI_Comm_size)(MPI_Comm, int*);
extern int (*libMPI_Comm_rank)(MPI_Comm, int*);
extern int (*libMPI_Finalize)(void);
extern int (*libMPI_Initialized)(int*);
extern int (*libMPI_Abort)(MPI_Comm, int);

extern int (*libMPI_Cancel)(MPI_Request*);

extern int (*libMPI_Send)(CONST void* buf, int count, MPI_Datatype datatype,
                          int dest, int tag, MPI_Comm comm);
extern int (*libMPI_Recv)(void* buf, int count, MPI_Datatype datatype,
                          int source, int tag, MPI_Comm comm,
                          MPI_Status* status);

extern int (*libMPI_Bsend)(CONST void*, int, MPI_Datatype, int, int, MPI_Comm);
extern int (*libMPI_Ssend)(CONST void*, int, MPI_Datatype, int, int, MPI_Comm);
extern int (*libMPI_Rsend)(CONST void*, int, MPI_Datatype, int, int, MPI_Comm);
extern int (*libMPI_Isend)(CONST void*, int, MPI_Datatype, int, int, MPI_Comm,
                           MPI_Request*);
extern int (*libMPI_Ibsend)(CONST void*, int, MPI_Datatype, int, int, MPI_Comm,
                            MPI_Request*);
extern int (*libMPI_Issend)(CONST void*, int, MPI_Datatype, int, int, MPI_Comm,
                            MPI_Request*);
extern int (*libMPI_Irsend)(CONST void*, int, MPI_Datatype, int, int, MPI_Comm,
                            MPI_Request*);
extern int (*libMPI_Irecv)(void*, int, MPI_Datatype, int, int, MPI_Comm,
                           MPI_Request*);

extern int (*libMPI_Sendrecv)(CONST void*, int, MPI_Datatype, int, int, void*,
                              int, MPI_Datatype, int, int, MPI_Comm,
                              MPI_Status*);
extern int (*libMPI_Sendrecv_replace)(void*, int, MPI_Datatype, int, int, int,
                                      int, MPI_Comm, MPI_Status*);

extern int (*libMPI_Send_init)(CONST void*, int, MPI_Datatype, int, int,
                               MPI_Comm, MPI_Request*);
extern int (*libMPI_Bsend_init)(CONST void*, int, MPI_Datatype, int, int,
                                MPI_Comm, MPI_Request*);
extern int (*libMPI_Ssend_init)(CONST void*, int, MPI_Datatype, int, int,
                                MPI_Comm, MPI_Request*);
extern int (*libMPI_Rsend_init)(CONST void*, int, MPI_Datatype, int, int,
                                MPI_Comm, MPI_Request*);
extern int (*libMPI_Recv_init)(void*, int, MPI_Datatype, int, int, MPI_Comm,
                               MPI_Request*);
extern int (*libMPI_Start)(MPI_Request*);
extern int (*libMPI_Startall)(int, MPI_Request*);

extern int (*libMPI_Wait)(MPI_Request*, MPI_Status*);
extern int (*libMPI_Test)(MPI_Request*, int*, MPI_Status*);
extern int (*libMPI_Waitany)(int, MPI_Request*, int*, MPI_Status*);
extern int (*libMPI_Testany)(int, MPI_Request*, int*, int*, MPI_Status*);
extern int (*libMPI_Waitall)(int, MPI_Request*, MPI_Status*);
extern int (*libMPI_Testall)(int, MPI_Request*, int*, MPI_Status*);
extern int (*libMPI_Waitsome)(int, MPI_Request*, int*, int*, MPI_Status*);
extern int (*libMPI_Testsome)(int, MPI_Request*, int*, int*, MPI_Status*);

extern int (*libMPI_Probe)(int source, int tag, MPI_Comm comm,
                           MPI_Status* status);
extern int (*libMPI_Iprobe)(int source, int tag, MPI_Comm comm, int* flag,
                            MPI_Status* status);

extern int (*libMPI_Barrier)(MPI_Comm);
extern int (*libMPI_Bcast)(void*, int, MPI_Datatype, int, MPI_Comm);
extern int (*libMPI_Gather)(CONST void*, int, MPI_Datatype, void*, int,
                            MPI_Datatype, int, MPI_Comm);
extern int (*libMPI_Gatherv)(CONST void*, int, MPI_Datatype, void*, CONST int*,
                             CONST int*, MPI_Datatype, int, MPI_Comm);
extern int (*libMPI_Scatter)(CONST void*, int, MPI_Datatype, void*, int,
                             MPI_Datatype, int, MPI_Comm);
extern int (*libMPI_Scatterv)(CONST void*, CONST int*, CONST int*,
                              MPI_Datatype, void*, int, MPI_Datatype, int,
                              MPI_Comm);
extern int (*libMPI_Allgather)(CONST void*, int, MPI_Datatype, void*, int,
                               MPI_Datatype, MPI_Comm);
extern int (*libMPI_Allgatherv)(CONST void*, int, MPI_Datatype, void*,
                                CONST int*, CONST int*, MPI_Datatype,
                                MPI_Comm);
extern int (*libMPI_Alltoall)(CONST void*, int, MPI_Datatype, void*, int,
                              MPI_Datatype, MPI_Comm);
extern int (*libMPI_Alltoallv)(CONST void*, CONST int*, CONST int*,
                               MPI_Datatype, void*, CONST int*, CONST int*,
                               MPI_Datatype, MPI_Comm);
extern int (*libMPI_Reduce)(CONST void*, void*, int, MPI_Datatype, MPI_Op, int,
                            MPI_Comm);
extern int (*libMPI_Allreduce)(CONST void*, void*, int, MPI_Datatype, MPI_Op,
                               MPI_Comm);
extern int (*libMPI_Reduce_scatter)(CONST void*, void*, CONST int*,
                                    MPI_Datatype, MPI_Op, MPI_Comm);
extern int (*libMPI_Scan)(CONST void*, void*, int, MPI_Datatype, MPI_Op,
                          MPI_Comm);

#ifdef USE_MPI3
extern int (*libMPI_Ibarrier)(MPI_Comm, MPI_Request*);
extern int (*libMPI_Ibcast)(void*, int, MPI_Datatype, int, MPI_Comm, MPI_Request*);
extern int (*libMPI_Igather)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm, MPI_Request*);
extern int (*libMPI_Igatherv)(const void*, int, MPI_Datatype, void*, const int*, const int*, MPI_Datatype, int, MPI_Comm, MPI_Request*);
extern int (*libMPI_Iscatter)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm, MPI_Request*);
extern int (*libMPI_Iscatterv)(const void*, const int*, const int*, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm, MPI_Request*);
extern int (*libMPI_Iallgather)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm, MPI_Request*);
extern int (*libMPI_Iallgatherv)(const void*, int, MPI_Datatype, void*, const int*, const int*, MPI_Datatype, MPI_Comm, MPI_Request*);
extern int (*libMPI_Ialltoall)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm, MPI_Request*);
extern int (*libMPI_Ialltoallv)(const void*, const int*, const int*, MPI_Datatype, void*, const int*, const int*, MPI_Datatype, MPI_Comm, MPI_Request*);
extern int (*libMPI_Ireduce)(const void*, void*, int, MPI_Datatype, MPI_Op, int, MPI_Comm, MPI_Request*);
extern int (*libMPI_Iallreduce)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm, MPI_Request*);
extern int (*libMPI_Ireduce_scatter)(const void*, void*, const int*, MPI_Datatype, MPI_Op, MPI_Comm, MPI_Request*);
extern int (*libMPI_Iscan)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm, MPI_Request*);
#endif

extern int (*libMPI_Get)(void*, int, MPI_Datatype, int, MPI_Aint, int,
                         MPI_Datatype, MPI_Win);
extern int (*libMPI_Put)(CONST void*, int, MPI_Datatype, int, MPI_Aint, int,
                         MPI_Datatype, MPI_Win);

extern int (*libMPI_Comm_spawn)(CONST char* command, char* argv[], int maxprocs,
                                MPI_Info info, int root, MPI_Comm comm,
                                MPI_Comm* intercomm, int array_of_errcodes[]);

/* fortran bindings */
extern void (*libmpi_init_)(int* e);
extern void (*libmpi_init_thread_)(int*, int*, int*);
extern void (*libmpi_finalize_)(int*);
extern void (*libmpi_barrier_)(MPI_Comm*, int*);
extern void (*libmpi_comm_size_)(MPI_Comm*, int*, int*);
extern void (*libmpi_comm_rank_)(MPI_Comm*, int*, int*);
extern void (*libmpi_cancel_)(MPI_Request*, int*);

extern void (*libmpi_send_)(void*, int*, MPI_Datatype*, int*, int*, int*);
extern void (*libmpi_recv_)(void*, int*, MPI_Datatype*, int*, int*,
                            MPI_Status*, int*);

extern void (*libmpi_sendrecv_)(void*, int, MPI_Datatype, int, int, void*,
                                int, MPI_Datatype, int, int, MPI_Comm,
                                MPI_Status*, int*);
extern void (*libmpi_sendrecv_replace_)(void*, int, MPI_Datatype, int, int, int,
                                        int, MPI_Comm, MPI_Status*, int*);

extern void (*libmpi_bsend_)(void*, int*, MPI_Datatype*, int*, int*, MPI_Comm*,
                             int*);
extern void (*libmpi_ssend_)(void*, int*, MPI_Datatype*, int*, int*, MPI_Comm*,
                             int*);
extern void (*libmpi_rsend_)(void*, int*, MPI_Datatype*, int*, int*, MPI_Comm*,
                             int*);
extern void (*libmpi_isend_)(void*, int*, MPI_Datatype*, int*, int*, MPI_Comm*,
                             MPI_Request*, int*);
extern void (*libmpi_ibsend_)(void*, int*, MPI_Datatype*, int*, int*, MPI_Comm*,
                              MPI_Request*, int*);
extern void (*libmpi_issend_)(void*, int*, MPI_Datatype*, int*, int*, MPI_Comm*,
                              MPI_Request*, int*);
extern void (*libmpi_irsend_)(void*, int*, MPI_Datatype*, int*, int*, MPI_Comm*,
                              MPI_Request*, int*);
extern void (*libmpi_irecv_)(void*, int*, MPI_Datatype*, int*, int*, MPI_Comm*,
                             MPI_Request*, int*);

extern void (*libmpi_wait_)(MPI_Request*, MPI_Status*, int*);
extern void (*libmpi_test_)(MPI_Request*, int*, MPI_Status*, int*);
extern void (*libmpi_waitany_)(int*, MPI_Request*, int*, MPI_Status*, int*);
extern void (*libmpi_testany_)(int*, MPI_Request*, int*, int*, MPI_Status*,
                               int*);
extern void (*libmpi_waitall_)(int*, MPI_Request*, MPI_Status*, int*);
extern void (*libmpi_testall_)(int*, MPI_Request*, int*, MPI_Status*, int*);
extern void (*libmpi_waitsome_)(int*, MPI_Request*, int*, int*, MPI_Status*,
                                int*);
extern void (*libmpi_testsome_)(int*, MPI_Request*, int*, int*, MPI_Status*,
                                int*);

extern void (*libmpi_probe_)(int* source, int* tag, MPI_Comm* comm,
                             MPI_Status* status, int* err);
extern void (*libmpi_iprobe_)(int* source, int* tag, MPI_Comm* comm, int* flag,
                              MPI_Status* status, int* err);

extern void (*libmpi_get_)(void*, int*, MPI_Datatype*, int*, MPI_Aint*, int*,
                           MPI_Datatype*, MPI_Win*, int*);
extern void (*libmpi_put_)(void*, int*, MPI_Datatype*, int*, MPI_Aint*, int*,
                           MPI_Datatype*, MPI_Win*, int*);

extern void (*libmpi_bcast_)(void*, int*, MPI_Datatype*, int*, MPI_Comm*, int*);
extern void (*libmpi_gather_)(void*, int*, MPI_Datatype*, void*, int*,
                              MPI_Datatype*, int*, MPI_Comm*, int*);
extern void (*libmpi_gatherv_)(void*, int*, MPI_Datatype*, void*, int*, int*,
                               MPI_Datatype*, int*, MPI_Comm*);
extern void (*libmpi_scatter_)(void*, int*, MPI_Datatype*, void*, int*,
                               MPI_Datatype*, int*, MPI_Comm*, int*);
extern void (*libmpi_scatterv_)(void*, int*, int*, MPI_Datatype*, void*, int*,
                                MPI_Datatype*, int*, MPI_Comm*, int*);
extern void (*libmpi_allgather_)(void*, int*, MPI_Datatype*, void*, int*,
                                 MPI_Datatype*, MPI_Comm*, int*);
extern void (*libmpi_allgatherv_)(void*, int*, MPI_Datatype*, void*, int*, int*,
                                  MPI_Datatype*, MPI_Comm*);
extern void (*libmpi_alltoall_)(void*, int*, MPI_Datatype*, void*, int*,
                                MPI_Datatype*, MPI_Comm*, int*);
extern void (*libmpi_alltoallv_)(void*, int*, int*, MPI_Datatype*, void*, int*,
                                 int*, MPI_Datatype*, MPI_Comm*, int*);
extern void (*libmpi_reduce_)(void*, void*, int*, MPI_Datatype*, MPI_Op*, int*,
                              MPI_Comm*, int*);
extern void (*libmpi_allreduce_)(void*, void*, int*, MPI_Datatype*, MPI_Op*,
                                 MPI_Comm*, int*);
extern void (*libmpi_reduce_scatter_)(void*, void*, int*, MPI_Datatype*,
                                      MPI_Op*, MPI_Comm*, int*);
extern void (*libmpi_scan_)(void*, void*, int*, MPI_Datatype*, MPI_Op*,
                            MPI_Comm*, int*);

extern void (*libmpi_comm_spawn_)(char* command, char** argv, int* maxprocs,
                                  MPI_Info* info, int* root, MPI_Comm* comm,
                                  MPI_Comm* intercomm, int* array_of_errcodes,
                                  int* error);

extern void (*libmpi_send_init_)(void*, int*, MPI_Datatype*, int*, int*,
                                 MPI_Comm*, MPI_Request*, int*);
extern void (*libmpi_bsend_init_)(void*, int*, MPI_Datatype*, int*, int*,
                                  MPI_Comm*, MPI_Request*, int*);
extern void (*libmpi_ssend_init_)(void*, int*, MPI_Datatype*, int*, int*,
                                  MPI_Comm*, MPI_Request*, int*);
extern void (*libmpi_rsend_init_)(void*, int*, MPI_Datatype*, int*, int*,
                                  MPI_Comm*, MPI_Request*, int*);
extern void (*libmpi_recv_init_)(void*, int*, MPI_Datatype*, int*, int*,
                                 MPI_Comm*, MPI_Request*, int*);
extern void (*libmpi_start_)(MPI_Request*, int*);
extern void (*libmpi_startall_)(int*, MPI_Request*, int*);

/* return 1 if buf corresponds to the Fotran MPI_IN_PLACE
 * return 0 otherwise
 */
int ezt_mpi_is_in_place_(void* buf);

/* check the value of a Fortran pointer and return MPI_IN_PLACE or p
 */
#define CHECK_MPI_IN_PLACE(p) (ezt_mpi_is_in_place_(p) ? MPI_IN_PLACE : (p))


#define PPTRACE_START_INTERCEPT_FUNCTIONS(module_name) struct ezt_instrumented_function INSTRUMENTED_FUNCTIONS [] = {

  
#define PPTRACE_END_INTERCEPT_FUNCTIONS(module_name) \
  FUNCTION_NONE					     \
  }                                        \
  ;

#define INTERCEPT3(func, var) {	\
    .function_name=func,			\
      .callback=&(var),				\
      .event_id = -1,				\
      },

#define FUNCTION_NONE  {			\
    .function_name="",				\
      .callback=NULL,				\
      .event_id = -1,				\
      },

static void instrument_function(struct ezt_instrumented_function* f) __attribute__((unused));
static void instrument_functions(struct ezt_instrumented_function* functions) __attribute__((unused));
static struct ezt_instrumented_function* find_instrumented_function(const char* fname, struct ezt_instrumented_function* functions) __attribute__((unused));

static void instrument_function(struct ezt_instrumented_function* f) {
  
  if(f->event_id >= 0) {
    /* this function has already been initialized */
    return;
  }

  assert(f->callback != NULL);

  static __thread int recursion_shield = 0;
  recursion_shield++;
  if(recursion_shield == 1) {
    if(*(void**)f->callback == NULL) {
      MPII_PRINTF(1, "Instrumenting %s using dlsym\n", f->function_name);
      /* binary instrumentation did not find the symbol. */
      void* ptr = dlsym(RTLD_NEXT, f->function_name);
      if(ptr) {
	memcpy(f->callback, &ptr, sizeof(void*));
      }
    }
  }
  recursion_shield--;
}

static struct ezt_instrumented_function* find_instrumented_function(const char* fname, struct ezt_instrumented_function* functions) {
  struct ezt_instrumented_function*f=NULL;
  for(f = functions;
      strcmp(f->function_name, "") != 0;
      f++) {
    if(strcmp(f->function_name, fname) == 0) {
      return f;
    }
  }
  return NULL;
}

static void instrument_functions(struct ezt_instrumented_function* functions) {
  struct ezt_instrumented_function*f=NULL;

  for(f = functions;
      strcmp(f->function_name, "") != 0;
      f++) {
    instrument_function(f);
  }
}

#define INSTRUMENT_ALL_FUNCTIONS() do {		\
  instrument_functions(INSTRUMENTED_FUNCTIONS);	\
  }while(0);


/* instrument one function */
#define INTERCEPT_FUNCTION(fname, cb) do {				\
    if(!cb) {								\
      struct ezt_instrumented_function*f=NULL;				\
      for(f = INSTRUMENTED_FUNCTIONS;					\
	  strcmp(f->function_name, fname) != 0 &&			\
	    strcmp(f->function_name, "") != 0;				\
	  f++) {							\
      }									\
      instrument_function(f);						\
    }									\
} while(0)


