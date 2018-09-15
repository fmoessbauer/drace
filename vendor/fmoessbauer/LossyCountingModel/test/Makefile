CXX=g++
FLAGS=-std=c++11 -Wall -Wpedantic
LIBINC=-I ../src
OUTPUT=lcm_test
FILES=main.cpp TestSuite.cpp

all: release

release: $(FILES)
	$(CXX) $(FLAGS) $(LIBINC) -O3 $(FILES) -o $(OUTPUT)

debug: $(FILES)
	$(CXX) $(FLAGS) -g $(LIBINC) $(FILES) -o $(OUTPUT)

clean:
	rm -rf *.o gmon.out $(OUTPUT)
