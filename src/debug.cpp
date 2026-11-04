#include "debug.h"
#include "containers.h"
#include "typedefs.h"
#include <execinfo.h>

#ifdef USE_BACKWARD
#define SKIP_FRAMES 4
#include <backward.hpp>
#include <ostream>
#include <sstream>
#endif

#ifndef STANDALONEDBG // if built with CPT
#include "parse_flags.h"
using namespace __cpt;

std::atomic<uint32_t> current_verbosity{0};

#else // for independent use of this file
struct debugFlags {
  FILE *output{stdout};
  bool abort_on_error{false};
  bool continue_on_error{true};
  int exitcode{-1};
};

debugFlags debugFlagsInstance;

inline debugFlags *get_cpt_flags() { return &debugFlagsInstance; }

#endif

// fast and safe way to print a stacktrace
void PrintStack() {
  FILE *out = (get_cpt_flags()->output ? get_cpt_flags()->output : stderr);
#ifdef USE_BACKWARD
  using namespace backward;
  StackTrace st;
  st.load_here(CALLSTACK_SIZE);
  st.skip_n_firsts(SKIP_FRAMES);
  Printer p;
  p.object = true;
  p.color_mode = ColorMode::automatic;
  p.address = true;
  p.print(st, out);
#else
  size_t size;
  void *array[CALLSTACK_SIZE];
  size = backtrace(array, CALLSTACK_SIZE);
  backtrace_symbols_fd(array, size, fileno(out));
#endif
}

// compose error message with stacktrace in a single buffer
void PrintStackBuffered(StackStreamBuffer &stream) {
#ifdef USE_BACKWARD
  using namespace backward;
  StackTrace st;
  st.load_here(CALLSTACK_SIZE);
  st.skip_n_firsts(SKIP_FRAMES);
  Printer p;
  p.object = true;
  p.color_mode = ColorMode::automatic;
  p.address = true;

  StringStream stringbuffer;
  p.print(st, stringbuffer);
  stream << stringbuffer.str().c_str() << "\n";
#else
  int nptrs;
  void *buf[CALLSTACK_SIZE + 1];
  nptrs = backtrace(buf, CALLSTACK_SIZE);
  char **symbols = backtrace_symbols(buf, nptrs);
  if (!symbols) {
    stream << "Stack trace failed\n";
    return;
  }
  for (int i = 0; i < nptrs; i++) {
    stream << symbols[i] << "\n";
  }
  free(symbols);
#endif
}

void NORETURN Die() {
  if (get_cpt_flags()->abort_on_error)
    abort();
  exit(get_cpt_flags()->exitcode);
}

void CheckFailed(const char *file, int line, const char *cond, u64 v1, u64 v2,
                 std::initializer_list<const char *> msgs) {
  char buffer[DBG_BUFFER_SIZE];
  StackStreamBuffer stream(buffer, DBG_BUFFER_SIZE);
  FILE *out = (get_cpt_flags()->output ? get_cpt_flags()->output : stderr);

  stream << "\nCheck failed in " << file << ":" << line << " "
         << (unsigned long long)v1 << " " << cond << " "
         << (unsigned long long)v2 << "\n";

  for (auto &m : msgs) {
    stream << m;
  }

  PrintStackBuffered(stream);
  stream.fflush(out);

  if (!get_cpt_flags()->continue_on_error) {
    Die();
  }
}

// std::atomic needs this function in debug config
#ifndef USE_STL
namespace std {
extern "C++" _GLIBCXX_NORETURN __attribute__((__cold__)) void
    __glibcxx_assert_fail /* Called when a precondition violation is detected.
                           */
    (const char *__file, int __line, const char *__function,
     const char *__condition) _GLIBCXX_NOEXCEPT {
  CheckFailed(__file, __line, __condition, 0, 0, {__function});
  abort(); // this function should be noreturn
}
} // namespace std
#endif