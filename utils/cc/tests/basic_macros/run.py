#!/usr/bin/python

program = cc('program.c')
program.no_errors()
program.outputs('expected')

