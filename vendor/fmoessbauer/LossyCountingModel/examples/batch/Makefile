CXX=g++
FLAGS=-std=c++11 -fopenmp -Wall -Wpedantic
LIBINC=-I ../../src
OUTPUT=lcm_batch
FILES=main.cpp

all: release

release: $(FILES)
	$(CXX) $(FLAGS) $(LIBINC) -O3 -Ofast -march=native -D_GLIBCXX_PARALLEL $(FILES) -o $(OUTPUT)
	
debug: $(FILES)
	$(CXX) $(FLAGS) -g $(LIBINC) $(FILES) -o $(OUTPUT)

clean:
	rm -rf *.o gmon.out $(OUTPUT)
