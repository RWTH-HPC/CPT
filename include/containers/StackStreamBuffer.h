#pragma once

#include <cstdio>
#include <cstring>

namespace __cpt {

struct StackStreamBuffer {
  char *buf;
  size_t cap;
  size_t pos = 0;

  // this struct always needs a buffer
  StackStreamBuffer() = delete;

  StackStreamBuffer(char *_buf, size_t _capacity) : buf(_buf), cap(_capacity) {}

  StackStreamBuffer &operator<<(const char *s) {
    if (!s || pos > cap - 1 || !buf)
      return *this;
    pos += snprintf(buf + pos, cap - pos, "%s", s);
    pos = (pos < cap - 1) ? pos : cap - 1;
    return *this;
  }

  StackStreamBuffer &operator<<(unsigned long long n) {
    if (pos > cap - 1 || !buf)
      return *this;
    pos += snprintf(buf + pos, cap - pos, "%llu", (unsigned long long)n);
    pos = (pos < cap - 1) ? pos : cap - 1;
    return *this;
  }

  StackStreamBuffer &operator<<(int n) {
    if (pos > cap - 1 || !buf)
      return *this;
    pos += snprintf(buf + pos, cap - pos, "%d", (int)n);
    pos = (pos < cap - 1) ? pos : cap - 1;
    return *this;
  }

  StackStreamBuffer &fflush(FILE *out) {
    if (pos > cap - 1 || !buf)
      return *this;
    fwrite(buf, sizeof(char), pos, out);
    return *this;
  }

  StackStreamBuffer &reset() {
    if (!buf)
      return *this;

    memset(buf, 0, cap);
    pos = 0;
    return *this;
  }
};

} // namespace __cpt