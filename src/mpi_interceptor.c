#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <argp.h>
#include "mpii_config.h"

const char prefix[] = INSTALL_PREFIX;

const char *program_version = "mpi_interceptor";
const char *program_bug_address = "";
static char doc[] = "mpi_interceptor description";
static char args_doc[] = "target_application [TARGET OPTIONS]";
const char * argp_program_version="MPII dev";


// long name, key, arg, option flags, doc, group
// if key is negative or non printable, no short option
static struct argp_option options[] = {
	{0, 0, 0, 0, "Output options:"},
	{"verbose", 'v', 0, 0, "Produce verbose output" },
	{"force", 'f', 0, 0, "Force the use of thread-safety (even if MPI already supports it)" },
	{"disable", 'd', 0, 0, "Disable the use of thread-safety" },
	{"show", 's', 0, 0, "Show the LD_PRELOAD command to run the application with instrumentation" },
	{"check", 'c', 0, 0, "Check if the application performs concurrent MPI calls" },
	{"check-abort", 'C', 0, 0, "Abort if the concurrency check fails" },
	{0}
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  /* Get the input settings from argp_parse, which we
   * know is a pointer to our settings structure. */
  struct mpii_settings *settings = state->input;

  switch(key) {
  case 'v':
    settings->verbose = 1;
    break;
  case 's':
    settings->show = 1;
    break;
  case 'f':
    settings->force_thread_safety = 1;
    break;
  case 'd':
    settings->disable_thread_safety = 1;
    break;
  case 'c':
    settings->check_concurrency = 1;
    break;
  case 'C':
    /* automatically enable concurrency checks */
    settings->abort_on_concurrency_check_failure = 1;
    settings->check_concurrency = 1;
    break;

  case ARGP_KEY_NO_ARGS:
    argp_usage(state);
    break;
  case ARGP_KEY_ARG:
  case ARGP_KEY_END:
    // nothing to do
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };


int main(int argc, char**argv) {
  struct mpii_settings settings;
  
  // Default values
  settings.verbose = SETTINGS_VERBOSE_DEFAULT;
  settings.show = SETTINGS_SHOW_DEFAULT;
  settings.check_concurrency = SETTINGS_CHECK_CONCURRENCY_DEFAULT;
  settings.force_thread_safety = SETTINGS_FORCE_THREAD_SAFETY_DEFAULT;
  settings.disable_thread_safety = SETTINGS_DISABLE_THREAD_SAFETY_DEFAULT;
  settings.abort_on_concurrency_check_failure = SETTINGS_ABORT_ON_CONCURRENCY_CHECK_FAILURE_DEFAULT;

  // first divide argv between mpii options and target file and
  // options optionnal todo : better target detection : it should be
  // possible to specify both --option=value and --option value, but
  // for now the latter is not interpreted as such
  int target_i = 1;
  while (target_i < argc && argv[target_i][0] == '-') target_i++;
  if (target_i == argc) {
    // there are no settings, either the user entered --help or
    // something like that, either we want to print usage anyway
    return argp_parse(&argp, argc, argv, 0, 0, &settings);
  }
  	
  char **target_argv = NULL;
  if (target_i < argc)
    target_argv = &(argv[target_i]);
  // we only want to parse what comes before target included
  argp_parse(&argp, target_i+1, argv, 0, 0, &settings);

  char ld_preload[STRING_LENGTH] = "";
  char *str;
  if ((str = getenv("LD_PRELOAD")) != NULL) {
    strncpy(ld_preload, str, STRING_LENGTH);
    strcat(ld_preload, ":");
  }
  strcat(ld_preload, prefix);
  strcat(ld_preload, "/lib/libmpi-interceptor.so");
	
  setenv("LD_PRELOAD", ld_preload, 1);

#define setenv_int(var, value, overwrite) do {	\
    char str[STRING_LENGTH];			\
    snprintf(str, STRING_LENGTH, "%d", value);	\
    setenv(var, str, overwrite);		\
  }while(0)
  
  setenv_int("MPII_VERBOSE", settings.verbose, 1);
  setenv_int("MPII_CHECK_CONCURRENCY", settings.check_concurrency, 1);
  setenv_int("MPII_FORCE_THREAD_SAFETY", settings.force_thread_safety, 1);
  setenv_int("MPII_DISABLE_THREAD_SAFETY", settings.disable_thread_safety, 1);
  setenv_int("MPII_ABORT_ON_CONCURRENCY_CHECK_FAILURE", settings.abort_on_concurrency_check_failure, 1);


  if(settings.show) {
    setenv("LD_PRELOAD", ld_preload, 1);
    printf("LD_PRELOAD=%s MPII_VERBOSE=%d MPII_FORCE_THREAD_SAFETY=%d MPII_DISABLE_THREAD_SAFETY=%d MPII_CHECK_CONCURRENCY=%d MPII_ABORT_ON_CONCURRENCY_CHECK_FAILURE=%d",
	   ld_preload,
	   settings.verbose,
	   settings.force_thread_safety,
	   settings.disable_thread_safety,
	   settings.check_concurrency,
	   settings.abort_on_concurrency_check_failure);

    for(int i=target_i; i<argc; i++)
      printf(" %s", argv[i]);
    printf("\n");
    return EXIT_SUCCESS;    
  }

  extern char** environ;
  int ret;
  if (target_argv != NULL) {
    ret  = execve(argv[target_i], target_argv, environ);
  } else {
    char *no_argv[] = {NULL};
    ret = execve(argv[target_i], no_argv, environ);
  }
  // execve failed
  fprintf(stderr, "Could not execve : %d - %s\n", errno, strerror(errno));
  return EXIT_FAILURE;


}
