#ifndef INCLUDE_UTILS_TRACE_H
#define INCLUDE_UTILS_TRACE_H

#include <stdio.h>
#include <stdint.h>

#define TRACE_STR(variable) do { fprintf(stderr, __FILE__":%d "#variable"=%s\n", __LINE__, variable); } while (0)
#define TRACE_INT(variable) do { fprintf(stderr, __FILE__":%d "#variable"=%d\n", __LINE__, variable); } while (0)
#define TRACE_PTR(variable) do { fprintf(stderr, __FILE__":%d "#variable"=0x%016lx\n", __LINE__, (uint64_t)(variable)); } while (0)
#define TRACE_SIZ(variable) do { fprintf(stderr, __FILE__":%d "#variable"=%zu\n", __LINE__, variable); } while (0)

#endif  // INCLUDE_UTILS_TRACE_H
