# This is just to test weather the code compiles when developing under Linux.

CXXFLAGS += -std=c++14 \
            -Ibwapi/bwapi/include \
            -IBWEM/include \
            -Wall \
            -Wno-unknown-pragmas

SOURCES := $(filter-out src/DllMain.cpp,$(wildcard src/*.cpp))
OBJECTS := $(addprefix obj/,$(notdir $(SOURCES:.cpp=.o)))

.PHONY: all
all: obj $(OBJECTS)

obj:
	mkdir obj

obj/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

.PHONY: tidy
tidy:
	clang-tidy -checks=cppcoreguidelines-*,modernize-*,readability-* \
	    -header-filter=src/ $(SOURCES) -- $(CXXFLAGS)

.PHONY: clean
clean:
	rm -rf obj
