#define PETSC_DESIRE_FEATURE_TEST_MACROS /* for fileno() */
#include <petscsys.h>                    /*I "petscsys.h" I*/
#include <petsc/private/petscimpl.h>
#include <petscconfiginfo.h>
#if defined(PETSC_HAVE_UNISTD_H)
  #include <unistd.h>
#endif
#include "err.h"
#include <petsc/private/logimpl.h> // PETSC_TLS

/*@C
  PetscIgnoreErrorHandler - Deprecated, use `PetscReturnErrorHandler()`. Ignores the error, allows program to continue as if error did not occur

  Not Collective

  Input Parameters:
+ comm - communicator over which error occurred
. line - the line number of the error (indicated by __LINE__)
. fun  - the function name
. file - the file in which the error was detected (indicated by __FILE__)
. mess - an error text string, usually just printed to the screen
. n    - the generic error number
. p    - specific error number
- ctx  - error handler context

  Level: developer

  Note:
  Users do not directly call this routine

.seealso: `PetscReturnErrorHandler()`
 @*/
PetscErrorCode PetscIgnoreErrorHandler(MPI_Comm comm, int line, const char *fun, const char *file, PetscErrorCode n, PetscErrorType p, const char *mess, void *ctx)
{
  (void)comm;
  (void)line;
  (void)fun;
  (void)file;
  (void)p;
  (void)mess;
  (void)ctx;
  return n;
}

/* ---------------------------------------------------------------------------------------*/

static char      arch[128], hostname[128], username[128], pname[PETSC_MAX_PATH_LEN], date[128];
static PetscBool PetscErrorPrintfInitializeCalled = PETSC_FALSE;
static char      version[256];

/*
   Initializes arch, hostname, username, date so that system calls do NOT need
   to be made during the error handler.
*/
PetscErrorCode PetscErrorPrintfInitialize(void)
{
  PetscBool use_stdout = PETSC_FALSE, use_none = PETSC_FALSE;

  PetscFunctionBegin;
  PetscCall(PetscGetArchType(arch, sizeof(arch)));
  PetscCall(PetscGetHostName(hostname, sizeof(hostname)));
  PetscCall(PetscGetUserName(username, sizeof(username)));
  PetscCall(PetscGetProgramName(pname, sizeof(pname)));
  PetscCall(PetscGetDate(date, sizeof(date)));
  PetscCall(PetscGetVersion(version, sizeof(version)));

  PetscCall(PetscOptionsGetBool(NULL, NULL, "-error_output_stdout", &use_stdout, NULL));
  if (use_stdout) PETSC_STDERR = PETSC_STDOUT;
  PetscCall(PetscOptionsGetBool(NULL, NULL, "-error_output_none", &use_none, NULL));
  if (use_none) PetscErrorPrintf = PetscErrorPrintfNone;
  PetscErrorPrintfInitializeCalled = PETSC_TRUE;
  PetscFunctionReturn(PETSC_SUCCESS);
}

PetscErrorCode PetscErrorPrintfNone(const char format[], ...)
{
  return PETSC_SUCCESS;
}

PetscErrorCode PetscErrorPrintfDefault(const char format[], ...)
{
  va_list          Argp;
  static PetscBool PetscErrorPrintfCalled = PETSC_FALSE;
  PetscErrorCode   ierr;

  /*
      This function does not call PetscFunctionBegin and PetscFunctionReturn() because
    it may be called by PetscStackView().

      This function does not do error checking because it is called by the error handlers.
  */

  if (!PetscErrorPrintfCalled) {
    PetscErrorPrintfCalled = PETSC_TRUE;

    /*
        On the SGI machines and Cray T3E, if errors are generated  "simultaneously" by
      different processors, the messages are printed all jumbled up; to try to
      prevent this we have each processor wait based on their rank
    */
#if defined(PETSC_CAN_SLEEP_AFTER_ERROR)
    {
      PetscMPIInt rank = PetscGlobalRank > 8 ? 8 : PetscGlobalRank;
      ierr             = PetscSleep((PetscReal)rank);
      (void)ierr;
    }
#endif
  }

  ierr = PetscFPrintf(PETSC_COMM_SELF, PETSC_STDERR, "[%d]PETSC ERROR: ", PetscGlobalRank);
  va_start(Argp, format);
  ierr = (*PetscVFPrintf)(PETSC_STDERR, format, Argp);
  (void)ierr;
  va_end(Argp);
  return PETSC_SUCCESS;
}

/*
   On some systems when the stderr is nested through several levels of shell script
   before being passed to a file the isatty() falsely returns true resulting in
   the screen highlight variables being passed through the test harness. Therefore
   simply do not highlight when the PETSC_STDERR is PETSC_STDOUT.
*/
static void PetscErrorPrintfHilight(void)
{
#if defined(PETSC_HAVE_UNISTD_H) && defined(PETSC_USE_ISATTY)
  if (PetscErrorPrintf == PetscErrorPrintfDefault && PETSC_STDERR != PETSC_STDOUT) {
    if (isatty(fileno(PETSC_STDERR))) fprintf(PETSC_STDERR, "\033[1;31m");
  }
#endif
}

static void PetscErrorPrintfNormal(void)
{
#if defined(PETSC_HAVE_UNISTD_H) && defined(PETSC_USE_ISATTY)
  if (PetscErrorPrintf == PetscErrorPrintfDefault && PETSC_STDERR != PETSC_STDOUT) {
    if (isatty(fileno(PETSC_STDERR))) fprintf(PETSC_STDERR, "\033[0;39m\033[0;49m");
  }
#endif
}

PETSC_EXTERN PetscErrorCode PetscOptionsViewError(void);

static PETSC_TLS PetscBool petsc_traceback_error_silent = PETSC_FALSE;

/*@C

  PetscTraceBackErrorHandler - Default error handler routine that generates
  a traceback on error detection.

  Not Collective

  Input Parameters:
+ comm - communicator over which error occurred
. line - the line number of the error (usually indicated by `__LINE__` in the calling routine)
. fun  - the function name
. file - the file in which the error was detected (usually indicated by `__FILE__` in the calling routine)
. mess - an error text string, usually just printed to the screen
. n    - the generic error number
. p    - `PETSC_ERROR_INITIAL` if this is the first call the error handler, otherwise `PETSC_ERROR_REPEAT`
- ctx  - error handler context

  Options Database Keys:
+ -error_output_stdout - output the error messages to `stdout` instead of the default `stderr`
- -error_output_none   - do not output the error messages

  Notes:
  Users do not directly call this routine

  Use `PetscPushErrorHandler()` to set the desired error handler.

  Level: developer

.seealso: `PetscError()`, `PetscPushErrorHandler()`, `PetscPopErrorHandler()`, `PetscAttachDebuggerErrorHandler()`,
          `PetscAbortErrorHandler()`, `PetscMPIAbortErrorHandler()`, `PetscReturnErrorHandler()`, `PetscEmacsClientErrorHandler()`,
           `PETSC_ERROR_INITIAL`, `PETSC_ERROR_REPEAT`, `PetscErrorCode`, `PetscErrorType`
 @*/
PetscErrorCode PetscTraceBackErrorHandler(MPI_Comm comm, int line, const char *fun, const char *file, PetscErrorCode n, PetscErrorType p, const char *mess, void *ctx)
{
  PetscErrorCode ierr;
  PetscMPIInt    rank = 0;

  (void)ctx;
  if (comm != PETSC_COMM_SELF) MPI_Comm_rank(comm, &rank);

  // reinitialize the error handler when a new initializing error is detected
  if (p != PETSC_ERROR_REPEAT) {
    petsc_traceback_error_silent = PETSC_FALSE;
    if (PetscCIEnabledPortableErrorOutput) {
      PetscMPIInt size = 1;

      if (comm != MPI_COMM_NULL) MPI_Comm_size(comm, &size);
      petscabortmpifinalize = (size == PetscGlobalSize) ? PETSC_TRUE : PETSC_FALSE;
    }
  }

  if (rank == 0 && (!PetscCIEnabledPortableErrorOutput || PetscGlobalRank == 0) && (p != PETSC_ERROR_REPEAT || !petsc_traceback_error_silent)) {
    static int cnt = 1;

    if (p == PETSC_ERROR_INITIAL) {
      PetscErrorPrintfHilight();
      ierr = (*PetscErrorPrintf)("--------------------- Error Message --------------------------------------------------------------\n");
      PetscErrorPrintfNormal();
      if (cnt > 1) {
        ierr = (*PetscErrorPrintf)("  It appears a new error in the code was triggered after a previous error, possibly because:\n");
        ierr = (*PetscErrorPrintf)("  -  The first error was not properly handled via (for example) the use of\n");
        ierr = (*PetscErrorPrintf)("     PetscCall(TheFunctionThatErrors()); or\n");
        ierr = (*PetscErrorPrintf)("  -  The second error was triggered while handling the first error.\n");
        ierr = (*PetscErrorPrintf)("  Above is the traceback for the previous unhandled error, below the traceback for the next error\n");
        ierr = (*PetscErrorPrintf)("  ALL ERRORS in the PETSc libraries are fatal, you should add the appropriate error checking to the code\n");
        cnt  = 1;
      }
    }
    if (cnt == 1) {
      if (n == PETSC_ERR_MEM || n == PETSC_ERR_MEM_LEAK) ierr = PetscErrorMemoryMessage(n);
      else {
        const char *text;
        ierr = PetscErrorMessage(n, &text, NULL);
        if (text) ierr = (*PetscErrorPrintf)("%s\n", text);
      }
      if (mess) ierr = (*PetscErrorPrintf)("%s\n", mess);
      ierr = PetscOptionsLeftError();
      ierr = (*PetscErrorPrintf)("See https://petsc.org/release/faq/ for trouble shooting.\n");
      if (!PetscCIEnabledPortableErrorOutput) {
        ierr = (*PetscErrorPrintf)("%s\n", version);
        if (PetscErrorPrintfInitializeCalled) ierr = (*PetscErrorPrintf)("%s on a %s named %s by %s %s\n", pname, arch, hostname, username, date);
        ierr = (*PetscErrorPrintf)("Configure options %s\n", petscconfigureoptions);
      }
    }
    /* print line of stack trace */
    if (fun) ierr = (*PetscErrorPrintf)("#%d %s() at %s:%d\n", cnt++, fun, PetscCIFilename(file), PetscCILinenumber(line));
    else if (file) ierr = (*PetscErrorPrintf)("#%d %s:%d\n", cnt++, PetscCIFilename(file), PetscCILinenumber(line));
    if (fun) {
      PetscBool ismain = PETSC_FALSE;

      ierr = PetscStrncmp(fun, "main", 4, &ismain);
      if (ismain) {
        if ((n <= PETSC_ERR_MIN_VALUE) || (n >= PETSC_ERR_MAX_VALUE)) ierr = (*PetscErrorPrintf)("Reached the main program with an out-of-range error code %d. This should never happen\n", n);
        ierr = PetscOptionsViewError();
        PetscErrorPrintfHilight();
        ierr = (*PetscErrorPrintf)("----------------End of Error Message -------send entire error message to petsc-maint@mcs.anl.gov----------\n");
        PetscErrorPrintfNormal();
      }
    }
  } else {
    // silence this process's stacktrace if it is not the root of an originating error
    if (p != PETSC_ERROR_REPEAT && rank) petsc_traceback_error_silent = PETSC_TRUE;
    if (fun) {
      PetscBool ismain = PETSC_FALSE;

      ierr = PetscStrncmp(fun, "main", 4, &ismain);
      if (ismain && petsc_traceback_error_silent) {
        /* This results from PetscError() being called in main: PETSCABORT()
           will be called after the error handler.  But this thread is not the
           root rank of the communicator that initialized the error.  So sleep
           to allow the root thread to finish its printing.

           (Unless this is running CI, in which case do not sleep because
           we expect all processes to call MPI_Finalize() and make a clean
           exit.) */
        if (!PetscCIEnabledPortableErrorOutput) ierr = PetscSleep(10.0);
      }
    }
  }
  (void)ierr;
  return n;
}
