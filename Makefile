SOURCES = $(wildcard src/*.c)
OBJECTS = $(patsubst src/%.c, target/%.o, $(SOURCES))
INCLUDE = -I $(PWD)/include -I /usr/include/cjson/
LINKS   = -L target/release
LIBS    = -lpthread -ldl -lcjson -lcontainer_images
DEFINES = -DNO_NSUSER -DCGROUP_V1
FLAGS   = -std=gnu11 -fPIC
OUT_DIR = $(PWD)/target/
RUST    = target/libcontainer_images.a
TARGET  = container

ifeq ($(CONFIG_OVERLAY),y)
	DEFINES += -DOVERLAY_ROOTFS 
endif

ifeq ($(CONFIG_IMAGE),y)
	DEFINES += -DIMAGE_SUPPORT
endif

ifeq ($(CONFIG_DEBUG),y)
	DEFINES += -DDEBUG_INFO
	FLAGS += -g -O0 -w
else
	FLAGS += -O2 -w 
endif

all: $(TARGET)

.PHONY: default fmt container clean run exec exit build

$(TARGET): $(OBJECTS)
	@echo "Compiling Rust module..."
	@cargo build --release
	@cbindgen -lC -o include/bindings/container_images.h libs/container-images/src/lib.rs
	@echo "Compiling container"
	@gcc $(LINKS) $(DEFINES) $(FLAGS) $^ -o $(OUT_DIR)/$@ $(LIBS)

$(RUST):
	cargo build --release
	@cbindgen -lC -o include/bindings/container_images.h libs/container-images/src/lib.rs
	@cp target/release/libcontainer_images.a target/libcontainer_images.a

target/%.o: src/%.c
	@echo "Compiling $@"
	@gcc $(LINKS) $(DEFINES) $(FLAGS) $(INCLUDE) -c $< -o $@ $(LIBS)

run:
	@rm -rf /tmp/demo-container
	@$(OUT_DIR)/$(TARGET) run nginx

build:
	@rm -rf /tmp/demo-container
	@$(OUT_DIR)/$(TARGET) build nginx

exec:
	@$(OUT_DIR)/$(TARGET) exec

exit:
	@$(OUT_DIR)/$(TARGET) exit

fmt:
	fd '.*\.c' -X clang-format -i {}
	fd '.*\.h' -X clang-format -i {}
	# @fd '.*\..*~' -X rm {}
	@fd -ec -eh -ers -emd -x cat {} | wc -l

clean:
	@fd -eo -egch -X rm {}
	@rm -rf $(OUT_DIR)/*.o $(OUT_DIR)/$(TARGET) $(RUST)

default:
	@make CONFIG_OVERLAY=y CONFIG_IMAGE=y 
