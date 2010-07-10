#!/usr/bin/env python

"""
This script runs the chronos firmware through the debugger msp430-debugger
step by step until the firmware jumps until a watermark (means it restarted)
.
It then stops so you see the traceback of PC addresses
"""

WATERMARK = 0x800a
outbuffer = []

import subprocess
import os, re, sys

pop = subprocess.Popen(["mspdebug","rf2500"], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
pop.stdin.write("sym import build/eZChronos.dbg.elf\n")

INPUT_LINE = re.compile(r'''\(mspdebug\) .*''')
PC_MATCH = re.compile(r'''PC: ([0-9a-fA-F]+)''')

started = False

def loop_until():
    global started
    i = 0
    while True:
        buf = pop.stdout.readline()
        #sys.stdout.write(buf)
        outbuffer.append(buf)
        search = PC_MATCH.search(buf)
        if search:
            pos = int(search.group(1), 16)
            if pos >= WATERMARK and not started:
                started = True
                print " ####################################### STARTED #################################"
            if pos < WATERMARK and started:
                print " ############################### UNDER WATERMARK DETECTED ########################"
                # print outbuffer
                for line in outbuffer:
                    sys.stdout.write(line)
                return
            print search.group(0)
            if i == 10:
                # trunkate outbuffer to 10000 lines
                del outbuffer[:len(outbuffer)-10000]

        if INPUT_LINE.match(buf):
            pop.stdin.write("step\n")
            #sys.stdout.write("step\n")
        i = i+1%5000


try:
    while True:
        loop_until()
        ignore = False
        while True:
            buf = pop.stdout.readline()
            sys.stdout.write(buf)
            if INPUT_LINE.match(buf):
                #if ignore:
                #    ignore = False
                #else:
                    command = raw_input(">>> ")
                    if command == "restart":
                        started = False
                        pop.stdin.write("step\n")
                        sys.stdout.write("step\n")
                        break
                    if command == "continue":
                        pop.stdin.write("step\n")
                        sys.stdout.write("step\n")
                        break
                    elif command == "exit":
                        raise KeyboardInterrupt
                    else:
                        pop.stdin.write("%s\n" %command)
                        ignore = True
        
    #pop.stdin.write("step\n")
except KeyboardInterrupt:
    print "kill %s" %pop.pid
    pop.terminate()
