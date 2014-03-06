
ifdef BAG
$(SACK_BASE)/makefiles/$(RINTDEST)/mm: mmf.c
	gcc -DBAG -o $@ -D__LINUX__ mmf.c
else
$(SACK_BASE)/makefiles/$(RINTDEST)/mm: mmf.c
	gcc -o $@ -D__LINUX__ mmf.c
endif

makefile.lnx: ;
