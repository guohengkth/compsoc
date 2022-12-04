CFLAGS := -g

all: fractal
	./fractal output.bmp

fractal: fractal.c
	gcc -Wall --pedantic -o fractal $(CFLAGS) $^ -lm

clean:
	rm -rf fractal output.bmp *~ *.dSYM

.PHONY: clean

