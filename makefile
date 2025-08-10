src_files = $(shell find src -iname "*.cpp")
obj_files = $(src_files:src/%.cpp=obj/%.o)


ifeq ($(OS),Windows_NT)
    CCFLAGS += -D WIN32 -Iext/PTHREADS-BUILT/include
    PTHREADLIB = ext/PTHREADS-BUILT/lib/pthreadVCE3.lib
    ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
        CCFLAGS += -D AMD64
    else
        ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
            CCFLAGS += -D AMD64
        endif
        ifeq ($(PROCESSOR_ARCHITECTURE),x86)
            CCFLAGS += -D IA32
        endif
    endif
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        CCFLAGS += -D LINUX
    endif
    ifeq ($(UNAME_S),Darwin)
        CCFLAGS += -D OSX
    endif
    UNAME_P := $(shell uname -p)
    ifeq ($(UNAME_P),x86_64)
        CCFLAGS += -D AMD64
    endif
    ifneq ($(filter %86,$(UNAME_P)),)
        CCFLAGS += -D IA32
    endif
    ifneq ($(filter arm%,$(UNAME_P)),)
        CCFLAGS += -D ARM
    endif
endif

debug: OPTIMIZATION_FLAG = -g
release: OPTIMIZATION_FLAG = -O3

release: all export_comp_db

debug: all export_comp_db

all: $(obj_files)
	@ mkdir -p bin
	g++ -I inc/ $^ $(PTHREADLIB) -o bin/main $(OPTIMIZATION_FLAG) $(CCFLAGS)

obj/%.o: src/%.cpp
	@ mkdir -p obj
	g++ -I inc/ -c $< -o $@ $(OPTIMIZATION_FLAG) $(CCFLAGS)

export_comp_db:
	echo [ > compile_commands.json
	make debug -B --dry-run > temp
	awk '/g\+\+.*\.cpp/ { f="compile_commands.json"; printf "\t\{\n\t\t\"directory\": \"%s\",\n\t\t\"command\": \"%s\",\n\t\t\"file\": \"%s\"\n\t\},\n", ENVIRON["PWD"], $$0, $$5 >> f }' temp
	echo ] >> compile_commands.json
	rm temp

clean:
	rm -f obj/*.o bin/*.exe
