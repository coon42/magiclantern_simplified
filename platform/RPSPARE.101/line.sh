#!/bin/bash

addr2line -s -e magiclantern -b elf32-little -f $1
