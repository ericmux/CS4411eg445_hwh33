
# Makefile for the PortOS project

# the target that is built when you run "make" with no arguments:
default: all

# the targets that are built when you run "make all"
#    targets are built from the target name followed by ".c".  For example, to
#    build "sieve", this Makefile will compile "sieve.c", along with all of the
#    necessary PortOS code.
#
# this would be a good place to add your tests
all: p1_tests/test1 p1_tests/test2 p1_tests/test3 p1_tests/test_1000 p1_tests/buffer p1_tests/sieve p1_tests/retail p2_tests/itest1 p2_tests/isleepsort p2_tests/ialarm network1 network2 network3 network4 network5 network6


# running "make clean" will remove all files ignored by git.  To ignore more
# files, you should add them to the file .gitignore
clean:
	git clean -fdX

################################################################################
# Everything below this line can be safely ignored.

CC     = gcc
CFLAGS = -mno-red-zone -fno-omit-frame-pointer -g -O0 -I. \
         -Wdeclaration-after-statement -Wall -Werror
LFLAGS = -lrt -pthread -g

OBJ =                              \
    minithread.o                   \
    interrupts.o                   \
    machineprimitives.o            \
    machineprimitives_x86_64.o     \
    machineprimitives_x86_64_asm.o \
    random.o                       \
    alarm.o                        \
    queue.o                        \
    synch.o                        \
    miniheader.o                   \
    minimsg.o                      \
    multilevel_queue.o             \
    network.o                      \
    hashtable.o                    \
    linked_list.o

%: %.o start.o end.o $(OBJ) $(SYSTEMOBJ)
	$(CC) $(LIB) -o $@ start.o $(filter-out start.o end.o $(SYSTEMOBJ), $^) end.o $(SYSTEMOBJ) $(LFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

machineprimitives_x86_64_asm.o: machineprimitives_x86_64_asm.S
	$(CC) -c machineprimitives_x86_64_asm.S -o machineprimitives_x86_64_asm.o

.depend:
	gcc -MM *.c > .depend

.SUFFIXES:
.PHONY: default all clean

include .depend
