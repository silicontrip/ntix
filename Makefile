# migrated to llvm-mingw https://github.com/mstorsjo/llvm-mingw

CC = x86_64-w64-mingw32-clang
CXX = x86_64-w64-mingw32-clang++
#CFLAGS = -target x86_64-w64-windows-gnu -fsanitize=address -static -Wall -g -I. -D_CRT_SECURE_NO_WARNINGS -DNTIX_STATIC -Wl,--subsystem,console
CFLAGS = -target x86_64-w64-windows-gnu -Wall -O2 -I. -D_CRT_SECURE_NO_WARNINGS -DNTIX_STATIC
CXXFLAGS = -std=c++17 -stdlib=libc++

CXX_LDFLAGS= -lm -static

LIBS = -lntdll

CPP_CORE_OBJS = cpp/narguments.o cpp/nexception.o cpp/nstring.o cpp/npath.o cpp/NtixCoreLib.o cpp/NtixLoader.o cpp/NtixObjectLib.o cpp/NtixFileLib.o


TEST = bin/test_cpp.exe 

TOOLS = \
	bin/mount.exe \
	bin/ls.exe 

all: setup $(TOOLS)

setup:
	@mkdir -p bin

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp %.hpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c $< -o $@

bin/test_cpp.exe: cpp/test_cpp.cpp $(CPP_CORE_OBJS)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $^ -o $@  $(CXX_LDFLAGS) $(LIBS)

bin/ls.exe: cpp/ls.cpp $(CPP_CORE_OBJS)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $^ -o $@ $(CXX_LDFLAGS) $(LIBS)

bin/mount.exe: cpp/mount.cpp $(CPP_CORE_OBJS)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $^ -o $@ $(CXX_LDFLAGS) $(LIBS)


clean:
	rm -f bin/*.exe cpp/*.o

rebuild: clean all

homedev: all
	scp -r bin 10.101.222.65:

.PHONY: all clean rebuild setup
