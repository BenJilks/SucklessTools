#!/usr/bin/python

program = cc('program.c')
program.no_errors()
program.returns(42)

program = cc('program_error.c')
program.has_errors()

