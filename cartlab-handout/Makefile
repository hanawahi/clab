#
# CMPSC311 - F19 Project 4: cartlab 
# Makefile - makefile for the assignment
#

# Locations
CMPSC311_LIBDIR=.

# Make environment
# INCLUDES=-I. -I$(CMPSC311_LIBDIR)
CC=gcc
CFLAGS=-I. -c -Og -g -Wall $(INCLUDES)
LINKARGS=-g
LIBS=-lcartlib -lcmpsc311 -lgcrypt -lcurl -L$(CMPSC311_LIBDIR) 

# Files
OBJECT_FILES= cart_sim.o cart_driver.o
                    
# Productions
all : cart_sim

cart_sim : $(OBJECT_FILES)
	$(CC) $(LINKARGS) $(OBJECT_FILES) -o $@ $(LIBS)

clean : 
	rm -f cart_sim $(OBJECT_FILES) workload/*.cmm
	
