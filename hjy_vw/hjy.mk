BINARIES = a 

CXX = g++
#VWLIBS := -L. -l vw -l allreduce
#STDLIBS = $(BOOST_LIBRARY) $(LIBS)

%.o:	 %.cc  %.h
	$(CXX) $(FLAGS) -c $< -o $@

%.o:	 %.cc
	$(CXX) $(FLAGS) -c $< -o $@

a: a.o 
#	$(CXX) $(FLAGS) -o $@ $< $(VWLIBS) $(STDLIBS)
	$(CXX) $(FLAGS) -o $@ $< 

clean:
	rm -f  *.o $(BINARIES) 
