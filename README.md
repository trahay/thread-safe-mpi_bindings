# Thread-safe MPI bindings

TL;DR: This is a library that intercepts an application calls to MPI, and make them thread safe.

## How ?

MPI provides several thread-safet support, ranging from
MPI_THREAD_SINGLE (only one thread in the application), to
MPI_THREAD_MULTIPLE (the application may have several thread that call
MPI concurrently).

Some MPI implementations do not provide the MPI_THREAD_MULTIPLE
thread-safety level. In that case, the application has to make sure
that threads to not call MPI concurrently, by using a mutex for
example. But modifying the application can be complex.

This library aims at solving this issue. It intercepts MPI calls and use a mutex to make sure that threads do not enter MPI concurrently



## Running an application


```bash
mpirun mpi_interceptor [options] myappli
```


The following options can be passed to MPI Interceptor:

- `-v` or `--verbose`
  + Set verbosity level (default level: 0)
- `-f` or `--force`
  + Force thread-safety (default: disabled) -- Even if the MPI implementation does provide MPI_THREAD_MULTIPLE, or if the application does not need MPI_THREAD_MULTIPLE, this option provides thread-safety. It can be used for testing the library

- `-d` or `--disable`
  + Disable thread-safety (default: no) -- Disable the library thread-safety mecanism. It can be used for testing the library instrumentation.



## Status of the current implementation

The current implementation intercepts the following functions and make them thread-safe:


- MPI_Init_thread: yes
- MPI_Init: yes
- MPI_Finalize: yes
- MPI_Barrier: yes
- MPI_Comm_size: yes
- MPI_Comm_rank: yes
- MPI_Comm_get_parent: yes
- MPI_Type_size: yes
- MPI_Cancel: yes
- MPI_Comm_disconnect: yes
- MPI_Comm_free: yes
- MPI_Comm_create: yes
- MPI_Comm_create_group: yes
- MPI_Comm_split: yes
- MPI_Comm_dup: yes
- MPI_Comm_dup_with_info: yes
- MPI_Comm_split_type: yes
- MPI_Intercomm_create: yes
- MPI_Intercomm_merge: yes
- MPI_Cart_sub: yes
- MPI_Cart_create: yes
- MPI_Graph_create: yes
- MPI_Dist_graph_create: yes
- MPI_Dist_graph_create_adjacent: yes
- MPI_Send: yes
- MPI_Recv: yes
- MPI_Sendrecv: More or less (*)
- MPI_Sendrecv_replace: More or less (*)
- MPI_Bsend: yes
- MPI_Ssend: yes
- MPI_Rsend: yes
- MPI_Isend: yes
- MPI_Ibsend: yes
- MPI_Issend: yes
- MPI_Irsend: yes
- MPI_Irecv: yes
- MPI_Wait: yes
- MPI_Waitall: yes
- MPI_Waitany: yes
- MPI_Waitsome: yes
- MPI_Test: yes
- MPI_Testall: yes
- MPI_Testany: yes
- MPI_Testsome: yes
- MPI_Iprobe: yes
- MPI_Probe: yes
- MPI_Get: More or less (*)
- MPI_Put: More or less (*)
- MPI_Bcast: yes
- MPI_Gather: yes
- MPI_Gatherv: yes
- MPI_Scatter: yes
- MPI_Scatterv: yes
- MPI_Allgather: yes
- MPI_Allgatherv: yes
- MPI_Alltoall: yes
- MPI_Alltoallv: yes
- MPI_Reduce: yes
- MPI_Allreduce: yes
- MPI_Reduce_scatter: yes
- MPI_Scan: yes
- MPI_Ibarrier: yes
- MPI_Ibcast: yes
- MPI_Igather: yes
- MPI_Igatherv: yes
- MPI_Iscatter: yes
- MPI_Iscatterv: yes
- MPI_Iallgather: yes
- MPI_Iallgatherv: yes
- MPI_Ialltoall: yes
- MPI_Ialltoallv: yes
- MPI_Ireduce: yes
- MPI_Iallreduce: yes
- MPI_Ireduce_scatter: yes
- MPI_Iscan: yes


(*) Some functions (such as `MPI_Put`, `MPI_Get`, or `MPI_Sendrecv`)
are blocking and don't have non-blocking counterpart. For these
function, our library takes a lock while entering, and release it
after the function call. This ensures that threads do not enter MPI
concurrently, but this may lead to deadlocks.
