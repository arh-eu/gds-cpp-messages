CXX = g++

CXX_INCLUDE_PATHS = -I./includes -isystem./submodules/asio/asio/include -isystem./submodules/msgpack/include -isystem./submodules/simple-websocket -I$(SOURCE_DIR)
CXX_VERSION = -std=c++17

CXX_FLAGS_DEFAULT = -Ofast -Wall -fPIC -Wall $(CXX_VERSION) $(CXX_INCLUDE_PATHS) \
	-Wextra \
	-Wmisleading-indentation \
	-Wmissing-braces \
	-Wduplicated-cond \
	-Wunused-parameter \
	-Wsuggest-override \
	-Wbool-compare \
    -Wtautological-compare \
    -Wzero-as-null-pointer-constant \
    -Wfloat-conversion \
    -Wdouble-promotion \
    -Wshadow \
    -Wswitch \
    -Werror=return-type

CXX_FLAGS_SHARED = $(CXX_FLAGS_DEFAULT) -shared  
CXX_FLAGS_STATIC = $(CXX_FLAGS_DEFAULT) -static

ASIO_DECL := -DASIO_STANDALONE

LD_FLAGS_SHARED = -lssl -lcrypto -lpthread

SOURCE_DIR = ./src
OUTPUT_DIR = ./output
BUILD_DIR = ./build
LIB_DIR = $(OUTPUT_DIR)/lib
INCLUDE_DIR = $(OUTPUT_DIR)/include

PATH_SHARED = $(LIB_DIR)/libgds.so
PATH_STATIC = $(LIB_DIR)/libgds.a

SOURCE_FILES = $(shell find $(SOURCE_DIR) -type f -name "*.cpp")
SOURCE_INTER := $(SOURCE_FILES:%.cpp=%.o)
SOURCE_OBJECTS := $(SOURCE_INTER:$(SOURCE_DIR)%=$(BUILD_DIR)%)

all: static shared

static: copy_includes $(BUILD_DIR) $(PATH_STATIC)

# rule that creates 
$(PATH_STATIC): $(SOURCE_OBJECTS) $(LIB_DIR)
	ar rcs $(PATH_STATIC) $(SOURCE_OBJECTS)

# build shared library
shared: copy_includes $(LIB_DIR) $(BUILD_DIR) $(SOURCE_OBJECTS)
	$(CXX) $(CXX_FLAGS_SHARED) $(SOURCE_OBJECTS) -o $(PATH_SHARED) $(LD_FLAGS_SHARED)

# copy necessary includes
copy_includes: $(INCLUDE_DIR)
#	cp $(SOURCE_DIR)/gds_lib_config.hpp $(INCLUDE_DIR)
#	cp $(SOURCE_DIR)/gds_lib_interface.hpp $(INCLUDE_DIR)
	cp $(SOURCE_DIR)/gds_connection.hpp $(INCLUDE_DIR)
	cp $(SOURCE_DIR)/gds_types.hpp $(INCLUDE_DIR)
	cp $(SOURCE_DIR)/semaphore.hpp $(INCLUDE_DIR)
	
.PHONY: clean all static shared
clean: 
	rm -rf $(OUTPUT_DIR) $(BUILD_DIR)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(LIB_DIR): $(OUTPUT_DIR)
	mkdir -p $(LIB_DIR)

$(INCLUDE_DIR): $(OUTPUT_DIR)
	mkdir -p $(INCLUDE_DIR)

# Build source objs
$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	@echo "[COMPILING] "$<" --> "$@
	@$(CXX) -c $(CXX_FLAGS_DEFAULT) $(ASIO_DECL) $< -o $@
