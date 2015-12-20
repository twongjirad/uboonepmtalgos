CXX=g++
CXXFLAGS = -g -I. 
CXXFLAGS += `root-config --cflags`
LDFLAGS = 
LDLIBS = `root-config --libs` -lstdc++


all: make_pe_spectra

make_pe_spectra.o:
	$(CXX) $(CXXFLAGS) -c make_pe_spectra.cc -o $@

make_pe_spectra: make_pe_spectra.o
	$(CXX) -o make_pe_spectra $^ $(LDLIBS)

clean:
	rm *.o make_pe_spectra
