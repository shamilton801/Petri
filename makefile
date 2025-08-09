src_files = $(find src -iname "*.cpp")
obj_files = $(src_files:%.cpp=%.o)

all: $(obj_files)
	echo $(obj_files)
	echo $(src_files)
	g++ -o main $^

%.o: %.cpp
	g++ -o $@ $<

clean:
	rm -f *.o *.exe
