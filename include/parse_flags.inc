#ifndef PARSE_FLAG
#error "Define PARSE_FLAG prior to including this file!"
#endif

// PARSE_FLAG(Type, Name, DefaultValue, Description)
// Supported types: bool, const char *, int, uptr.
// Default value must be a compile-time constant.
// Description must be a string literal.

PARSE_FLAG(bool, print_max_rss, false,
           "Print memory usage information at the end of execution.")

PARSE_FLAG(bool, verbose, false, "Provide verbose output.")

PARSE_FLAG(bool, enabled, true,
           "Inject OpenMP synchronization semantics to TSan.")

#ifdef USE_CUDA
PARSE_FLAG(bool, cuda_analysis, false, "Analyze CUDA activity")

PARSE_FLAG(bool, gpu_critical_path, false,
           "Analyze critical path through device activity")

PARSE_FLAG(int, device_limit, 16,
           "Maximum number of devices that can be tracked")

PARSE_FLAG(int, event_limit, 12,
           "Only in case of error: maximum number of events before "
           "aggregating, power of 2, so 2^(event_limit)")

PARSE_FLAG(int, cupti_buffer_limit, 20,
           "Only in case of error: CUPTI buffer limit, power of 2, so "
           "2^(cupti_buffer_limit)")
#endif

#ifdef USE_MPI
PARSE_FLAG(bool, mpi_analysis, true, "MPI Analysis")
#endif

PARSE_FLAG(bool, omp_analysis, true, "OpenMP Analysis")

PARSE_FLAG(bool, report_data_leak, false, "Report tool internal data leaks.")

PARSE_FLAG(bool, stopped, false, "Delay start of measurement.")

PARSE_FLAG(bool, start_with_library_constructor, false,
           "Start measurement with library constructor.")

PARSE_FLAG(bool, dump_on_stop, false, "Print output on stop.")

PARSE_FLAG(bool, barrier, false,
           "Synchronize start and stop of measurement with an MPI_Barrier.")

PARSE_FLAG(bool, colorize, true, "Colorize output.")

PARSE_FLAG(bool, help, false, "Print the flag descriptions.")

PARSE_FLAG(bool, stacktrace, false,
           "Print stacktraces in case of an error. "
           "Implies registering signal handlers and MPI error handlers.")

PARSE_FLAG(bool, abort_on_error, false, "Abort instead of exit on error.")

PARSE_FLAG(bool, continue_on_error, true, "Continue after failing an assert.")

PARSE_FLAG(int, metric_factor, 1, "Factor for metric output.")

PARSE_FLAG(int, exitcode, 23, "Exit code for errors.")

PARSE_FLAG(double, runtime, -1, "Real runtime from low overhead run.")

PARSE_FLAG(const char *, log_path, "stdout",
           "Write logs to \"log_path.pid\". The special values are \"stdout\" "
           "and \"stderr\".")

PARSE_FLAG(const char *, data_path, "stdout",
           "Write metric data to \"<data_path>-<#procs>x<#threads>.txt\". "
           "The special values are \"stdout\" and \"stderr\".")
