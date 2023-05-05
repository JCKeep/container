SOURCES = $(wildcard src/*.c)
OBJECTS = $(patsubst src/%.c, target/%.o, $(SOURCES))
INCLUDE = -I $(PWD)/include -I /usr/include/cjson/
LIBS    = -lcjson -larchive
DEFINES = -DNO_NSUSER -DCGROUP_v1
FLAGS   = -Wno-incompatible-pointer-types \
	-Wno-unused-result \
	-Wno-unused-label \
	-Wno-discarded-qualifiers \
	-std=gnu11 
OUT_DIR = $(PWD)/target/
RUST    = target/libcontainer_images.a
TARGET = container

ifeq ($(CONFIG_OVERLAY),y)
	DEFINES += -DOVERLAY_ROOTFS 
endif

ifeq ($(CONFIG_DEBUG),y)
	DEFINES += -DDEBUG_INFO
	FLAGS += -g -O0 -Wall
else
	FLAGS += -O2 -w 
endif

all: $(TARGET)

.PHONY: fmt container clean run exec exit prepare

$(TARGET): $(RUST) $(OBJECTS)
	gcc $(DEFINES) $^ -o $(OUT_DIR)/$@ $(LIBS)

$(RUST):
	cargo build --release
	cbindgen -lC -o include/bindings/container_images.h libs/container-images/src/lib.rs
	cp target/release/libcontainer_images.a target/libcontainer_images.a

target/%.o: src/%.c
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
	rm -rf $(OUT_DIR)

prepare:
	./scripts/cJSON
