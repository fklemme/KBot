# This is just to test weather the code compiles when developing under Linux.

CXXFLAGS += -std=c++11 \
            -IBWAPI/include \
            -IBWEM/include

SOURCES := $(filter-out src/DllMain.cpp,$(wildcard src/*.cpp))
OBJECTS := $(addprefix obj/,$(notdir $(SOURCES:.cpp=.o)))

.PHONY: all
all: obj $(OBJECTS)

obj:
	mkdir obj

obj/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

.PHONY: clean
clean:
	rm -rf obj
