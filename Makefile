# This is just to test weather the code compiles when developing under Linux.

CXXFLAGS += -std=c++14 \
            -Ibwapi/bwapi/include \
            -IBWEM/include \
            -Wall \
            -Wno-unknown-pragmas

HEADERS := $(wildcard src/*.h)
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

# Clang Format - Settings in file .clang-format
.PHONY: format
format:
	clang-format -i -style=file $(HEADERS) $(SOURCES)

# Clang Tidy - Removed some rules because...
#   - cppcoreguidelines-pro-bounds-array-to-pointer-decay: "assert" brings up a lot of these warnings.
#   - cppcoreguidelines-pro-type-vararg:                   We have to use Broodwar->drawText...(). :/
#   - readability-braces-around-statements:                I don't like that rule. Let's ignore these.
.PHONY: tidy
tidy:
	clang-tidy -checks=cppcoreguidelines-*,modernize-*,readability-*,-cppcoreguidelines-pro-bounds-array-to-pointer-decay,-cppcoreguidelines-pro-type-vararg,-readability-braces-around-statements \
	    -header-filter=src/ $(SOURCES) -- $(CXXFLAGS)
