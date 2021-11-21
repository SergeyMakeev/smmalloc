CXXFLAGS += -g -DNDEBUG -O3 -march=native
ARFLAGS = rvs

all:
	$(CXX) $(CXXFLAGS) -shared -fPIC -ftls-model=initial-exec gnuwrapper.cc -o libltalloc.so
	if ! $(CXX) $(CXXFLAGS) -flto -c gnuwrapper.cc -o libltalloc.o; then $(CXX) $(CXXFLAGS) -c gnuwrapper.cc -o libltalloc.o; fi
	$(AR) $(ARFLAGS) libltalloc.a libltalloc.o
	$(RM) libltalloc.o
