all : libgc.a

obj := gc.o

CFLAGS := -g -Wall -Werror

libgc.a : $(obj)
	ar cru $@ $^
	ranlib $@

%.o : %.c
	$(CC) $(CFLAGS) -MMD -c -o $@ $<

-include $(obj:.o=.d)

clean :
	rm -f libgc.a *.o *.d

.PHONY : all clean
