SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)
INCLUDE = $(PWD)/include
# DEFINES = -DMINUSRFS
FLAGS = -O2 -Wall -Wno-incompatible-pointer-types
OUT_DIR = $(PWD)/target/
TARGET = container

all: $(TARGET)

.PHONY: fmt container clean run

$(TARGET): $(OBJECTS)
	gcc $(DEFINES) $^ -o $(OUT_DIR)/$@
	fd -eo -egch -X rm {}

src/%.o: src/%.c
	gcc $(DEFINES) $(FLAGS) -I $(INCLUDE) -c $< -o $@

run:
	@rm -rf /tmp/demo-container
	@$(OUT_DIR)/$(TARGET)

fmt:
	fd '.*\.c' -X indent -npro -kr -i8 -ts8 -sob -l80 -ss -ncs -cp1 {}
	fd '.*\..*~' -X rm {}

clean:
	fd -eo -egch -X rm {}
	rm -f $(OBJECTS) $(OUT_DIR)/$(TARGET)
