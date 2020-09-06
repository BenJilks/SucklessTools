#ifndef X86_H
#define X86_H

#include "parser.h"
#include "x86_asm.h"

X86Code x86_compile_unit(Unit *unit);

#endif // X86_H
