##
## @file Makefile
## Verteilte Systeme simple_message_client
##
## @author Dominic Schebeck <ic17b049@technikum-wien.at>
## @author Thomas Neugschwandtner <ic17b082@technikum-wien.at>
## @date 2018/11/24
##
## @version 1.0
##
##
## ------------------------------------------------------------- variables --
##

CC=gcc
CFLAGS=-DDEBUG -Wall -pedantic -Werror -Wextra -Wstrict-prototypes -fno-common -g -O3 -std=gnu11
CP=cp
CD=cd
MV=mv
GREP=grep
DOXYGEN=doxygen

OBJECTS=simple_message_client.o

EXCLUDE_PATTERN=footrulewidth

##
## ----------------------------------------------------------------- rules --
##

%.o: %.c
	$(CC) $(CFLAGS) -c $<

##
## --------------------------------------------------------------- targets --
##

all: simple_message_client

simple_message_client: $(OBJECTS)
	$(CC) $(CFLAGS) simple_message_client.o -lsimple_message_client_commandline_handling -o my_simple_message_client

clean:
	$(RM) *.o *~ simple_message_client

distclean: clean
	$(RM) -r doc

 doc: html pdf

html:
	$(DOXYGEN) Doxyfile.dcf

pdf: html
	$(CD) doc/pdf && \
	$(MV) refman.tex refman_save.tex && \
	$(GREP) -v $(EXCLUDE_PATTERN) refman_save.tex > refman.tex && \
	$(RM) refman_save.tex && \
	make && \
	$(MV) refman.pdf refman.save && \
	$(RM) *.pdf *.html *.tex *.aux *.sty *.log *.eps *.out *.ind *.idx \
	      *.ilg *.toc *.tps Makefile && \
	$(MV) refman.save refman.pdf

##
## ---------------------------------------------------------- dependencies --
##

##
## =================================================================== eof ==
##
