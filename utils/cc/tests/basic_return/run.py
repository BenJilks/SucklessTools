#!/usr/bin/python

program = cc('program.c')
program.no_errors()
program.returns(42)

