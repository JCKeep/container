SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)
INCLUDE = -I $(PWD)/include -I /usr/include/cjson/
LIBS    = -lcjson
DEFINES = -DNO_NSUSER 
FLAGS   = -O2 -Wall \
	-Wno-incompatible-pointer-types \
	-Wno-unused-result \
	-Wno-unused-label \
	-Wno-discarded-qualifiers
OUT_DIR = $(PWD)/target/
TARGET = container

ifeq ($(FEATURES),overlay)
	DEFINES += -DOVERLAY_ROOTFS 
endif

all: $(TARGET)

.PHONY: fmt container clean run exec exit prepare

$(TARGET): $(OBJECTS)
	gcc $(DEFINES) $^ -o $(OUT_DIR)/$@ $(LIBS)
	fd -eo -egch -X rm {}

src/%.o: src/%.c
	gcc $(DEFINES) $(FLAGS) $(INCLUDE) -c $< -o $@ $(LIBS)

run:
	@rm -rf /tmp/demo-container
	@$(OUT_DIR)/$(TARGET) run

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

prepare:
	./scripts/cJSON
