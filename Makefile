#
# Stackistry: astronomical image stacking
#
# This file requires GNU Make (or a compatible). The C++ compiler has tu support
# dependencies file generation (see C_DEP_GEN_OPT and C_DEP_TARGET_OPT).
# Tested with GCC under Linux and MSYS.
#
# prerequisite libraries: gtkmm30-devel, libskry
#

#-------- User-configurable variables --------------------------

SKRY_INCLUDE_PATH = ../libskry/include
SKRY_LIB_PATH = ../libskry/bin

#---------------------------------------------------------------

CC = g++
CCFLAGS = -c -O3 -ffast-math -std=c++11 -fopenmp -Wno-parentheses -Wno-missing-field-initializers -Wall -Wextra -pedantic $(shell pkg-config gtkmm-3.0 --cflags) -I $(SKRY_INCLUDE_PATH)

# Cmdline option for CC to generate dependencies list to std. output
C_DEP_GEN_OPT = -MM

# Cmdline option for CC to specify target name of generated dependencies
C_DEP_TARGET_OPT = -MT

OBJ_DIR = ./obj
BIN_DIR = ./bin
SRC_DIR = ./src

# Command to create directory without error if existing, make parent directories as needed
MKDIR_P = mkdir -p

REMOVE = rm
OSTYPE = $(shell echo $$OSTYPE)

EXE_NAME = stackistry

SRC_FILES = config.cpp        \
            frame_select.cpp  \
            img_viewer.cpp    \
            main_window.cpp   \
            main.cpp          \
            preferences.cpp   \
            quality_wnd.cpp   \
            select_points.cpp \
            settings_dlg.cpp  \
            utils.cpp         \
            worker.cpp

# Converts the specified path $(1) to the form:
#   $(OBJ_DIR)/<filename>.o
#
define make_object_name_from_src_file_name =
$(addprefix $(OBJ_DIR)/, \
    $(patsubst %.cpp, %.o, \
        $(notdir $(1))))
endef
            
OBJECTS = \
$(foreach srcfile, $(SRC_FILES), \
    $(call make_object_name_from_src_file_name, $(srcfile)))
            
EXE_FLAGS =

ifeq ($(OSTYPE),msys)
OBJECTS += $(OBJ_DIR)/winres.o
# Prevents creating a console window
EXE_FLAGS += -Wl,--subsystem=windows
endif
          
all: directories $(BIN_DIR)/$(EXE_NAME)

directories:
	$(MKDIR_P) $(BIN_DIR)
	$(MKDIR_P) $(OBJ_DIR)

clean:
	$(REMOVE) -f ${OBJ_DIR}/*.o
	$(REMOVE) -f ${OBJ_DIR}/*.d
	$(REMOVE) -f $(BIN_DIR)/$(EXE_NAME)

$(BIN_DIR)/$(EXE_NAME): $(OBJECTS)
	$(CC) $(OBJECTS) $(shell pkg-config gtkmm-3.0 --libs) $(EXE_FLAGS) -L $(SKRY_LIB_PATH) -lskry -lgomp -s -o $(BIN_DIR)/$(EXE_NAME)

# Pull in dependency info for existing object files
-include $(OBJECTS:.o=.d)

$(OBJ_DIR)/winres.o: $(SRC_DIR)/winres.rc
	windres $(SRC_DIR)/winres.rc $(OBJ_DIR)/winres.o

#
# Creates a rule for building a .cpp file
#
# Parameters:
#   $(1) - full path to .o file
#   $(2) - full path to .cpp file
#   $(3) - additional compiler flags (may be empty)
#
define CPP_file_rule_template =
$(1): $(2)
	$(CC) $(CCFLAGS) -c $(2) $(3) -o $(1)
	$(CC) $(CCFLAGS) $(2) $(3) $(C_DEP_GEN_OPT) $(C_DEP_TARGET_OPT) $(1) > $(patsubst %.o, %.d, $(1))
endef

# Create build rules for all $(SRC_FILES)

$(foreach srcfile, $(SRC_FILES), \
  $(eval \
    $(call CPP_file_rule_template, \
      $(call make_object_name_from_src_file_name, $(srcfile)), \
      $(SRC_DIR)/$(srcfile), \
      )))
