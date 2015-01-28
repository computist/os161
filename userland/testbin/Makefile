#
# Makefile for src/testbin (sources for programs installed in /testbin)
#

TOP=../..
.include "$(TOP)/mk/os161.config.mk"


SUBDIRS=a3_malloc a2a_write a2a_read a2a_filetest a2a_forktest add add2 argtest badcall bigexec bigfile conman \
	crash ctest dirconc dirseek dirtest ehello eadd eadd2 \
	f_test factorial farm \
	faulter filetest forkbomb forktest frack guzzle hash \
	helloworld hog huge \
	kitchen malloctest matmult palin parallelvm psort quinthuge \
	quintmat quintsort randcall rmdirtest rmtest sink sort \
	sparsefile sty tail tictac triplehuge triplemat triplesort \
	zero

# But not:
#    userthreads    (no support in kernel API in base system)

.include "$(TOP)/mk/os161.subdir.mk"
