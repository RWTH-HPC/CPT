#ifndef PARSE_FLAGS_H
#define PARSE_FLAGS_H

// NOLINTBEGIN

#include "containers.h"
#include "debug.h"
#include "errorhandler.h"
#include "typedefs.h"
#include <atomic>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ANALYSIS_FLAGS "CPT_OPTIONS"
#define analysis_flags get_cpt_flags()
#define analysis_flag_output                                                   \
  (get_cpt_flags()->output ? get_cpt_flags()->output : stderr)

namespace cpt {

enum HandleSignalMode {
  kHandleSignalNo,
  kHandleSignalYes,
  kHandleSignalExclusive,
};

class FlagParser;
class FlagHandlerBase {
public:
  virtual bool Parse(const char *value) { return false; }
  // Write the C string representation of the current value (truncated to fit)
  // into the buffer of size `size`. Returns false if truncation occurred and
  // returns true otherwise.
  virtual bool Format(char *buffer, uptr size) {
    if (size > 0)
      buffer[0] = '\0';
    return false;
  }

protected:
  ~FlagHandlerBase() {}

  inline bool FormatString(char *buffer, uptr size, const char *str_to_use) {
    uptr num_symbols_should_write = snprintf(buffer, size, "%s", str_to_use);
    return num_symbols_should_write < size;
  }

  void *operator new(size_t size) { return malloc(size); }

  void operator delete(void *p) { free(p); }
  friend FlagParser;
};

template <typename T> class FlagHandler final : public FlagHandlerBase {
  T *t_;

public:
  explicit FlagHandler(T *t) : t_(t) {}
  bool Parse(const char *value) final;
  bool Format(char *buffer, uptr size) final;

  // define new/delete operator to avoid dependency to libstdc++
  void *operator new(size_t size) { return malloc(size); }
  void operator delete(void *p) { free(p); }
};

inline bool ParseBool(const char *value, bool *b) {
  if (strcmp(value, "0") == 0 || strcmp(value, "no") == 0 ||
      strcmp(value, "false") == 0) {
    *b = false;
    return true;
  }
  if (strcmp(value, "1") == 0 || strcmp(value, "yes") == 0 ||
      strcmp(value, "true") == 0) {
    *b = true;
    return true;
  }
  return false;
}

template <> inline bool FlagHandler<bool>::Parse(const char *value) {
  if (ParseBool(value, t_))
    return true;
  printf("ERROR: Invalid value for bool option: '%s'\n", value);
  return false;
}

template <> inline bool FlagHandler<bool>::Format(char *buffer, uptr size) {
  return FormatString(buffer, size, *t_ ? "true" : "false");
}

template <>
inline bool FlagHandler<HandleSignalMode>::Parse(const char *value) {
  bool b;
  if (ParseBool(value, &b)) {
    *t_ = b ? kHandleSignalYes : kHandleSignalNo;
    return true;
  }
  if (strcmp(value, "2") == 0 || strcmp(value, "exclusive") == 0) {
    *t_ = kHandleSignalExclusive;
    return true;
  }
  printf("ERROR: Invalid value for signal handler option: '%s'\n", value);
  return false;
}

template <>
inline bool FlagHandler<HandleSignalMode>::Format(char *buffer, uptr size) {
  uptr num_symbols_should_write = snprintf(buffer, size, "%d", *t_);
  return num_symbols_should_write < size;
}

template <> inline bool FlagHandler<const char *>::Parse(const char *value) {
  *t_ = value;
  return true;
}

template <>
inline bool FlagHandler<const char *>::Format(char *buffer, uptr size) {
  return FormatString(buffer, size, *t_);
}

template <> inline bool FlagHandler<int>::Parse(const char *value) {
  char *value_end;
  *t_ = strtoll(value, &value_end, 10);
  bool ok = *value_end == 0;
  if (!ok)
    printf("ERROR: Invalid value for int option: '%s'\n", value);
  return ok;
}

template <> inline bool FlagHandler<int>::Format(char *buffer, uptr size) {
  uptr num_symbols_should_write = snprintf(buffer, size, "%d", *t_);
  return num_symbols_should_write < size;
}

template <> inline bool FlagHandler<double>::Parse(const char *value) {
  char *value_end;
  *t_ = strtod(value, &value_end);
  bool ok = *value_end == 0;
  if (!ok)
    printf("ERROR: Invalid value for double option: '%s'\n", value);
  return ok;
}

template <> inline bool FlagHandler<double>::Format(char *buffer, uptr size) {
  uptr num_symbols_should_write = snprintf(buffer, size, "%lf", *t_);
  return num_symbols_should_write < size;
}

template <> inline bool FlagHandler<uptr>::Parse(const char *value) {
  char *value_end;
  *t_ = strtoll(value, &value_end, 10);
  bool ok = *value_end == 0;
  if (!ok)
    printf("ERROR: Invalid value for uptr option: '%s'\n", value);
  return ok;
}

template <> inline bool FlagHandler<uptr>::Format(char *buffer, uptr size) {
  uptr num_symbols_should_write = snprintf(buffer, size, "0x%zx", *t_);
  return num_symbols_should_write < size;
}

template <> inline bool FlagHandler<s64>::Parse(const char *value) {
  char *value_end;
  *t_ = strtoll(value, &value_end, 10);
  bool ok = *value_end == 0;
  if (!ok)
    printf("ERROR: Invalid value for s64 option: '%s'\n", value);
  return ok;
}

template <> inline bool FlagHandler<s64>::Format(char *buffer, uptr size) {
  uptr num_symbols_should_write = snprintf(buffer, size, "%lld", *t_);
  return num_symbols_should_write < size;
}

class FlagParser {
  static const int kMaxFlags = 200;
  struct Flag {
    const char *name;
    const char *desc;
    FlagHandlerBase *handler;
  } *flags_;
  int n_flags_;

  const char *buf_;
  uptr pos_;
  Vector<char *> pointers;

public:
  FlagParser();
  ~FlagParser();
  void RegisterHandler(const char *name, FlagHandlerBase *handler,
                       const char *desc);
  void ParseString(const char *s, const char *env_name = 0);
  void ParseStringFromEnv(const char *env_name);
  void PrintFlagDescriptions();

private:
  void fatal_error(const char *err);
  bool is_space(char c);
  void skip_whitespace();
  void parse_flags(const char *env_option_name);
  void parse_flag(const char *env_option_name);
  bool run_handler(const char *name, const char *value);
  char *ll_strndup(const char *s, uptr n);
};

template <typename T>
static void RegisterFlag(FlagParser *parser, const char *name, const char *desc,
                         T *var) {
  FlagHandler<T> *fh = new FlagHandler<T>(var);
  parser->RegisterHandler(name, fh, desc);
}

void ReportUnrecognizedFlags();

extern const char *SanitizerToolName;

struct CptFlags {
#define PARSE_FLAG(Type, Name, DefaultValue, Description) Type Name;
#include "parse_flags.inc"
#undef PARSE_FLAG

  bool running{false};
  FILE *output{stderr};
  FlagParser parser;

  void SetDefaults();
  void CopyFrom(const CptFlags &other);

  void *operator new(size_t size) { return malloc(size); }

  void operator delete(void *p) { free(p); }
};

extern CptFlags *cpt_flags_dont_use;

void InitializeCptFlags();

inline CptFlags *get_cpt_flags() {
  if (!cpt_flags_dont_use)
    InitializeCptFlags();
  return cpt_flags_dont_use;
}

} // namespace cpt

// NOLINTEND
#endif
