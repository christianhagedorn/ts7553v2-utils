PROGNAME = bounce-test

PWD		= $(shell pwd)
 
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)gcc

OBJS := main.o lcd-display.o

CFLAGS = $(INCLUDES) -Wall -O3

vpath %.c ../cairo-test

$(PROGNAME):  $(OBJS)	
	$(CC) $(OBJS) $(LDFLAGS) -o $@

%.obj : %.c 
	@$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@
   
clean:
	rm -f $(OBJS) $(PROGNAME)


