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

PARSE_FLAG(bool, report_data_leak, false, "Report tool internal data leaks.")

PARSE_FLAG(bool, stopped, false, "Delay start of measurement.")

PARSE_FLAG(bool, start_with_library_constructor, false,
           "Start measurement with library constructor.")

PARSE_FLAG(bool, dump_on_stop, false, "Print output on stop.")

PARSE_FLAG(bool, barrier, false,
           "Synchronize start of measurement with an MPI_Barrier.")

PARSE_FLAG(bool, colorize, true, "Colorize output.")

PARSE_FLAG(bool, tasking, true,
           "Enable tasking analysis (disable currently not supported).")

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

PARSE_FLAG(const char *, no_tool_runtime, "0.0", "Runtime without tool")

PARSE_FLAG(bool, collect_task_type_share, false, "Collect task type share.")

PARSE_FLAG(
    const char *, task_prio_shares, "",
    "List of (non-negative integers for) task priorities whose share should be "
    "collected. Semicolon separated. Empty string disables collection.")

PARSE_FLAG(const char *, task_name_shares, "",
           "List of task names whose share should be collected. Semicolon "
           "separated. Empty string disables collection.")

PARSE_FLAG(bool, collect_task_counts_on_cp, false,
           "Whether to collect the number of tasks on the critical path.")