SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)
INCLUDE = $(PWD)/include
DEFINES = -DNO_NSUSER 
FLAGS = -O2 -Wall -Wno-incompatible-pointer-types -Wno-unused-result -Wno-unused-label
OUT_DIR = $(PWD)/target/
TARGET = container

ifeq ($(FEATURES),overlay)
	DEFINES += -DOVERLAY_ROOTFS 
endif

all: $(TARGET)

.PHONY: fmt container clean run exec exit

$(TARGET): $(OBJECTS)
	gcc $(DEFINES) $^ -o $(OUT_DIR)/$@
	fd -eo -egch -X rm {}

src/%.o: src/%.c
	gcc $(DEFINES) $(FLAGS) -I $(INCLUDE) -c $< -o $@

run:
	@rm -rf /tmp/demo-container
	@$(OUT_DIR)/$(TARGET) run --deamon

exec:
	@$(OUT_DIR)/$(TARGET) exec

exit:
	@$(OUT_DIR)/$(TARGET) exit

fmt:
	fd '.*\.c' -X indent -npro -kr -i8 -ts8 -sob -l80 -ss -ncs -cp1 {}
	fd '.*\..*~' -X rm {}

clean:
	fd -eo -egch -X rm {}
	rm -f $(OBJECTS) $(OUT_DIR)/$(TARGET)
