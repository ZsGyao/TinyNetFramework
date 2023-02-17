//
// Created by zgys on 23-2-17.
//

#ifndef SYLAR_MACRO_H
#define SYLAR_MACRO_H

#include <string.h>
#include <assert.h>
#include "util.h"

#define SYLAR_ASSERT(x) \
      if(!(x)) {              \
          SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x \
                                            << "\nbacetrace:\n" \
                                            << sylar::BacktraceToString(100, 2, "    "); \
          assert(x);          \
      }

#define SYLAR_ASSERT2(x, w) \
      if(!(x)) {              \
          SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x \
                                            << "\n" << w     \
                                            << "\nbacetrace:\n" \
                                            << sylar::BacktraceToString(100, 2, "    "); \
          assert(x);          \
      }

#endif //SYLAR_MACRO_H
