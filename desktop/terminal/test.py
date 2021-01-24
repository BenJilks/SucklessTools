#!/usr/bin/python
import os, sys

width = 100
height = 20
color = 40

for i in range(5):
    print('\033[' + str(color + i) + 'm')
    for y in range(0, height):
        print(' ' * width)
    print('\033[m')

