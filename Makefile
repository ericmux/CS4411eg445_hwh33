
# Makefile for the PortOS project

# the target that is built when you run "make" with no arguments:
default: all

# the targets that are built when you run "make all"
#    targets are built from the target name followed by ".c".  For example, to
#    build "sieve", this Makefile will compile "sieve.c", along with all of the
#    necessary PortOS code.
#
# this would be a good place to add your tests
all: conn-network1 conn-network2 conn-network3 mkfs 


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
    read.o                         \
    synch.o                        \
    miniheader.o                   \
    minimsg.o                      \
    multilevel_queue.o             \
    minisocket.o 				   \
    miniroute.o                    \
    network.o                      \
    hashtable.o                    \
    disk.o 						   \
    minifile.o 					   \
    mini_mkfs.o 				   \
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
