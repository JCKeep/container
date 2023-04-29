SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)
INCLUDE = $(PWD)/include
FLAGS = -O2 -Wall -Wno-incompatible-pointer-types
TARGET = container
DIR = $(PWD)/target/

all: $(TARGET)

.PHONY: fmt container clean run

$(TARGET): $(OBJECTS)
	gcc $^ -o $(DIR)$@
	fd -eo -egch -X rm {}

src/%.o: src/%.c
	gcc ${FLAGS} -I ${INCLUDE} -c $< -o $@

run:
	@rm -rf /tmp/demo-container
	@$(DIR)/$(TARGET)

fmt:
	fd '.*\.c' -X indent -npro -kr -i8 -ts8 -sob -l80 -ss -ncs -cp1 {}
	fd '.*\..*~' -X rm {}

clean:
	fd -eo -egch -X rm {}
	rm -f $(OBJECTS) $(DIR)/$(TARGET)
