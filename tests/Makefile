TESTS = addsub hello var ptr lazy textrel regparm
all: $(addprefix test,$(TESTS)) libloader.so
COMMON_C_FLAGS = -g
.PRECIOUS: %.so

%.so: %.c hello.h
	gcc $(COMMON_C_FLAGS) -m32 -shared -fPIC -o $@ $< -nostdlib -Wl,--hash-style=sysv

textrel.so: textrel.c hello.h
	gcc $(COMMON_C_FLAGS) -m32 -shared -o $@ $< -nostdlib -Wl,--hash-style=sysv

test%: test%.c %.so
	gcc $(COMMON_C_FLAGS) -m32 -o $@ $< libloader.so -Wl,-rpath,.

test: all $(addprefix call-,$(TESTS))

call-%:
	-./test$*

clean:
	rm -f *.o $(addprefix test,$(TESTS)) $(addsuffix .so,$(TESTS))

libloader.so: ../loader.h ../loader.c ../debug.h ../loader_private.h ../lazy-resolve.s
	mkdir -p ../build
	cd ../build && cmake .. && $(MAKE)
	ln -sf ../build/libloader.so libloader.so
