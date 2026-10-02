#include "parse_flags.h"

using namespace cpt;

class UnknownFlags {
  static const int kMaxUnknownFlags = 20;
  const char *unknown_flags_[kMaxUnknownFlags];
  int n_unknown_flags_;

public:
  void Add(const char *name) {
    CHECK_LT(n_unknown_flags_, kMaxUnknownFlags);
    unknown_flags_[n_unknown_flags_++] = strdup(name);
  }

  void Report() {
    if (!n_unknown_flags_)
      return;
    printf("WARNING: %s found %d unrecognized flag(s):\n", SanitizerToolName,
           n_unknown_flags_);
    for (int i = 0; i < n_unknown_flags_; ++i)
      printf("    %s\n", unknown_flags_[i]);
    n_unknown_flags_ = 0;
  }
};

UnknownFlags unknown_flags;

void cpt::ReportUnrecognizedFlags() { unknown_flags.Report(); }

char *FlagParser::ll_strndup(const char *s, uptr n) {
  uptr len = strnlen(s, n);
  char *s2 = (char *)malloc(len + 1);
  memcpy(s2, s, len);
  s2[len] = 0;
  return s2;
}

void FlagParser::PrintFlagDescriptions() {
  char buffer[128];
  buffer[sizeof(buffer) - 1] = '\0';
  printf("Available flags for %s:\n", SanitizerToolName);
  for (int i = 0; i < n_flags_; ++i) {
    bool truncated = !(flags_[i].handler->Format(buffer, sizeof(buffer)));
    CHECK_EQ(buffer[sizeof(buffer) - 1], '\0');
    const char *truncation_str = truncated ? " Truncated" : "";
    printf("\t%s\n\t\t- %s (Current Value%s: %s)\n", flags_[i].name,
           flags_[i].desc, truncation_str, buffer);
  }
}

void FlagParser::fatal_error(const char *err) {
  printf("%s: ERROR: %s\n", SanitizerToolName, err);
  Die();
}

bool FlagParser::is_space(char c) {
  return c == ' ' || c == ',' || c == ':' || c == '\n' || c == '\t' ||
         c == '\r';
}

void FlagParser::skip_whitespace() {
  while (is_space(buf_[pos_]))
    ++pos_;
}

void FlagParser::parse_flag(const char *env_option_name) {
  uptr name_start = pos_;
  while (buf_[pos_] != 0 && buf_[pos_] != '=' && !is_space(buf_[pos_]))
    ++pos_;
  if (buf_[pos_] != '=') {
    if (env_option_name) {
      printf("%s: ERROR: expected '=' in %s\n", SanitizerToolName,
             env_option_name);
      Die();
    } else {
      fatal_error("expected '='");
    }
  }
  char *name = ll_strndup(buf_ + name_start, pos_ - name_start);

  uptr value_start = ++pos_;
  char *value;
  if (buf_[pos_] == '\'' || buf_[pos_] == '"') {
    char quote = buf_[pos_++];
    while (buf_[pos_] != 0 && buf_[pos_] != quote)
      ++pos_;
    if (buf_[pos_] == 0)
      fatal_error("unterminated string");
    value = ll_strndup(buf_ + value_start + 1, pos_ - value_start - 1);
    ++pos_; // consume the closing quote
  } else {
    while (buf_[pos_] != 0 && !is_space(buf_[pos_]))
      ++pos_;
    if (buf_[pos_] != 0 && !is_space(buf_[pos_]))
      fatal_error("expected separator or eol");
    value = ll_strndup(buf_ + value_start, pos_ - value_start);
  }

  bool res = run_handler(name, value);
  free(name);
  pointers.PushBack(value);
  if (!res)
    fatal_error("Flag parsing failed.");
}

void FlagParser::parse_flags(const char *env_option_name) {
  while (true) {
    skip_whitespace();
    if (buf_[pos_] == 0)
      break;
    parse_flag(env_option_name);
  }
}

void FlagParser::ParseStringFromEnv(const char *env_name) {
  const char *env = getenv(env_name);
  VPrintf(1, "%s: %s\n", env_name, env ? env : "<empty>");
  ParseString(env, env_name);
}

void FlagParser::ParseString(const char *s, const char *env_option_name) {
  if (!s)
    return;
  // Backup current parser state to allow nested ParseString() calls.
  const char *old_buf_ = buf_;
  uptr old_pos_ = pos_;
  buf_ = s;
  pos_ = 0;

  parse_flags(env_option_name);

  buf_ = old_buf_;
  pos_ = old_pos_;
}

bool FlagParser::run_handler(const char *name, const char *value) {
  for (int i = 0; i < n_flags_; ++i) {
    if (strcmp(name, flags_[i].name) == 0)
      return flags_[i].handler->Parse(value);
  }
  // Unrecognized flag. This is not a fatal error, we may print a warning later.
  unknown_flags.Add(name);
  return true;
}

void FlagParser::RegisterHandler(const char *name, FlagHandlerBase *handler,
                                 const char *desc) {
  CHECK_LT(n_flags_, kMaxFlags);
  flags_[n_flags_].name = name;
  flags_[n_flags_].desc = desc;
  flags_[n_flags_].handler = handler;
  ++n_flags_;
}

FlagParser::FlagParser() : n_flags_(0), buf_(nullptr), pos_(0) {
  flags_ = (Flag *)malloc(sizeof(Flag) * kMaxFlags);
}
FlagParser::~FlagParser() {
  for (int i = 0; i < n_flags_; i++)
    delete flags_[i].handler;
  free(flags_);
  for (auto &ptr : pointers)
    free(ptr);
}

CptFlags *cpt::cpt_flags_dont_use{nullptr};
const char *cpt::SanitizerToolName = "OFTCPT";

void CptFlags::SetDefaults() {
#define PARSE_FLAG(Type, Name, DefaultValue, Description) Name = DefaultValue;
#include "parse_flags.inc"
#undef PARSE_FLAG
}

void CptFlags::CopyFrom(const CptFlags &other) {
  memcpy(this, &other, sizeof(*this));
}

static void RegisterCptFlags(FlagParser *parser, CptFlags *f){
#define PARSE_FLAG(Type, Name, DefaultValue, Description)                      \
  RegisterFlag(parser, #Name, Description, &f->Name);
#include "parse_flags.inc"
#undef PARSE_FLAG
}

SANITIZER_INTERFACE_WEAK_DEF(const char *, cpt_default_options, void) {
  return "";
}

void cpt::InitializeCptFlags() {
  if (cpt::cpt_flags_dont_use)
    return;
  CptFlags *f = cpt::cpt_flags_dont_use = new CptFlags;
  f->SetDefaults();

  auto &parser = f->parser;
  RegisterCptFlags(&parser, f);

  parser.ParseString(cpt_default_options());
  parser.ParseStringFromEnv(ANALYSIS_FLAGS);

  ReportUnrecognizedFlags();
  if (f->help)
    parser.PrintFlagDescriptions();

  if (f->log_path && f->verbose) {
    if (strcmp("stdout", f->log_path) == 0) {
      f->output = stdout;
    } else if (strcmp("stderr", f->log_path) == 0) {
      f->output = stderr;
    } else {
      auto len = strlen(f->log_path);
      char *tempath = (char *)malloc(len + 15);
      sprintf(tempath, "%s.%i", f->log_path, getpid());
      f->output = fopen(tempath, "w");
      free(tempath);
    }
  }
}
