BINARIES = vw 

CXX = g++
LIBPATH = /usr/lib/x86_64-linux-gnu
VWLIBS = -l boost_program_options -l pthread 
#FLAGS = -Wall -D_FILE_OFFSET_BITS=64 -I/usr/include/boost 
FLAGS = -Wall  -msse2 -mfpmath=sse -march=x86-64 -O3 -ffast-math -Wno-deprecated -D_FILE_OFFSET_BITS=64 -I/usr/include/boost 

all: vw

io.o: stack.h

parse_args.o: io.h parse_regressor.h parse_example.h

parse_example.o: io.h hash.h

vw.o: parse_example.h parse_regressor.h parse_args.h

%.o: %.cc  %.h
	$(CXX) $(FLAGS) -c $< -o $@

vw: vw.o io.o hash.o parse_regressor.o parse_example.o parse_args.o 
	$(CXX) $(FLAGS) -L${LIBPATH} -o $@ $+ ${VWLIBS}

clean:
	rm -f  *.o $(BINARIES) 
	rm -rf doc/

doc:
	doxygen Doxyfile
