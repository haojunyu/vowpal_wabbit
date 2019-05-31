BINARIES = dg_hash.a 

CXX = gcc
LIBPATH = /usr/lib/x86_64-linux-gnu
VWLIBS = -l boost_program_options -l pthread 
FLAGS = -Wall  -msse2 -mfpmath=sse -march=x86-64 -O3 -ffast-math -Wno-deprecated -D_FILE_OFFSET_BITS=64 -I/usr/include/boost 

all: $(BINARIES)

%.o: %.cc  %.h
	$(CXX) $(FLAGS) -c $< -o $@

$(BINARIES): hash.o
	ar rcs -o $@ $<

clean:
	rm -f  *.o *.so $(BINARIES) 
	rm -rf build/

py:
	python setup.py build_ext --inplace
