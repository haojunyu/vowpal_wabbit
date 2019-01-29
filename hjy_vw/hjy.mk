BINARIES = vw 

CXX = g++
#VWLIBS := -L. -l vw -l allreduce
#STDLIBS = $(BOOST_LIBRARY) $(LIBS)
FLAGS =  

#%.o:	 %.cc  %.h
#	$(CXX) $(FLAGS) -c $< -o $@

#%.o:	 %.cc
#	$(CXX) $(FLAGS) -c $< -o $@
io.o: io.cc io.h stack.h
	$(CXX) $(FLAGS) -c $< -o $@

vw: io.o 
#	$(CXX) $(FLAGS) -o $@ $< $(VWLIBS) $(STDLIBS)
	$(CXX) $(FLAGS) -o $@ $< 

clean:
	rm -f  *.o $(BINARIES) 
	rm -rf doc/

doc:
	doxygen Doxyfile
