#!/bin/bash
gcc -g *.c -o adapter -D_FILE_OFFSET_BITS=64 -lfuse