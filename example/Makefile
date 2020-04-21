CXX = g++

CXX_INCLUDE_PATHS = -I$(GDS_INCLUDE_PATH) -I$(SOURCE_DIR) -isystem../submodules/asio/asio/include 
CXX_VERSION = -std=c++17

CXX_FLAGS = -Ofast -Wall $(CXX_VERSION) $(CXX_INCLUDE_PATHS) \
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

ASIO_DECL := -DASIO_STANDALONE

#LD_FLAGS = -L$(GDS_LIB_PATH) /home/viktor/Git/GdsLib/output/lib/libgds.a -lcrypto -lssl -lpthread -lrt -lm -ldl #static link

#LD_FLAGS = -L$(GDS_LIB_PATH) -lgds -lcrypto -lssl -lpthread #dynamic link
LD_FLAGS = -L$(GDS_LIB_PATH) ../output/lib/libgds.a -lcrypto -lssl -lpthread -lrt -lm -ldl #static link

NAME = main

GDS_INCLUDE_PATH = ../output/include
GDS_LIB_PATH = ../output/lib

SOURCE_DIR = .
SOURCE_FILES = $(shell find $(SOURCE_DIR) -type f -name "*.cpp")
OUTPUT_DIR = .

all: $(OUTPUT_DIR) $(SOURCE_FILES) 
	$(CXX) $(CXX_FLAGS) $(SOURCE_FILES) $(ASIO_DECL) -o $(OUTPUT_DIR)/$(NAME) $(LD_FLAGS)

.PHONY: clean
clean: 
	rm -rf $(OUTPUT_DIR)

$(OUTPUT_DIR):
	mkdir $(OUTPUT_DIR)