/**
 *  Copyright Notice:
 *  Copyright 2021-2022 DMTF. All rights reserved.
 *  License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/libspdm/blob/main/LICENSE.md
 **/

#include <stdarg.h>
#include <stdio.h>
#include <base.h>

#include "library/debuglib.h"

void libspdm_debug_assert(const char *file_name, size_t line_number,
                          const char *description)
{
    printf("%s:%zu ASSERT(%s)\n", file_name, line_number, description);
    while (1) { __asm__("nop"); }
}

void libspdm_debug_print(size_t error_level, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
}
