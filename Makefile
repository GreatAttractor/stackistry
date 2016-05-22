#
# Stackistry: astronomical image stacking
#
# This file requires GNU Make (or a compatible). The C++ compiler has tu support
# dependencies file generation (see CC_DEP_OPT). Tested with GCC under Linux and MSYS.
#
# prerequisite libraries: gtkmm30-devel, libskry
#

#-------- User-configurable variables --------------------------

SKRY_INCLUDE_PATH = ../libskry/include
SKRY_LIB_PATH = ../libskry/bin

#---------------------------------------------------------------

CC = g++
CCFLAGS = -c -O3 -ffast-math -std=c++11 -fopenmp -Wno-parentheses -Wno-missing-field-initializers -Wall -Wextra -pedantic $(shell pkg-config gtkmm-3.0 --cflags) -I $(SKRY_INCLUDE_PATH)
CC_DEP_OPT = -MM
OBJ_DIR = ./obj
BIN_DIR = ./bin
SRC_DIR = ./src
MKDIR_P = mkdir -p
REMOVE = rm
OSTYPE = $(shell echo $$OSTYPE)

EXE_NAME = stackistry

OBJECTS = config.o \
          frame_select.o \
          img_viewer.o \
          main_window.o \
          main.o \
          preferences.o \
          select_points.o \
          settings_dlg.o \
          utils.o \
          worker.o

EXE_FLAGS =

ifeq ($(OSTYPE),msys)
OBJECTS += winres.o
# Prevents creating a console window
EXE_FLAGS += -Wl,--subsystem=windows
endif
          
OBJECTS_PREF = $(addprefix $(OBJ_DIR)/,$(OBJECTS))

all: directories $(BIN_DIR)/$(EXE_NAME)

directories:
	$(MKDIR_P) $(BIN_DIR)
	$(MKDIR_P) $(OBJ_DIR)

clean:
	$(REMOVE) -f $(OBJECTS_PREF)
	$(REMOVE) -f $(OBJECTS_PREF:.o=.d)
	$(REMOVE) -f $(BIN_DIR)/$(EXE_NAME)

$(BIN_DIR)/$(EXE_NAME): $(OBJECTS_PREF)
	$(CC) $(OBJECTS_PREF) $(shell pkg-config gtkmm-3.0 --libs) $(EXE_FLAGS) -L $(SKRY_LIB_PATH) -lskry -lgomp -s -o $(BIN_DIR)/$(EXE_NAME)

# pull in dependency info for existing object files
-include $(OBJECTS_PREF:.o=.d)

$(OBJ_DIR)/winres.o: $(SRC_DIR)/winres.rc
	windres $(SRC_DIR)/winres.rc $(OBJ_DIR)/winres.o

$(OBJ_DIR)/config.o: $(SRC_DIR)/config.cpp
	$(CC)  $(CCFLAGS) $(SRC_DIR)/config.cpp -o $(OBJ_DIR)/config.o
	$(CC)  $(CC_DEP_OPT) $(CCFLAGS) $(SRC_DIR)/config.cpp > $(OBJ_DIR)/config.d

$(OBJ_DIR)/frame_select.o: $(SRC_DIR)/frame_select.cpp
	$(CC)  $(CCFLAGS) $(SRC_DIR)/frame_select.cpp -o $(OBJ_DIR)/frame_select.o
	$(CC)  $(CC_DEP_OPT) $(CCFLAGS) $(SRC_DIR)/frame_select.cpp > $(OBJ_DIR)/frame_select.d

$(OBJ_DIR)/img_viewer.o: $(SRC_DIR)/img_viewer.cpp
	$(CC)  $(CCFLAGS) $(SRC_DIR)/img_viewer.cpp -o $(OBJ_DIR)/img_viewer.o
	$(CC)  $(CC_DEP_OPT) $(CCFLAGS) $(SRC_DIR)/img_viewer.cpp > $(OBJ_DIR)/img_viewer.d

$(OBJ_DIR)/main_window.o: $(SRC_DIR)/main_window.cpp
	$(CC)  $(CCFLAGS) $(SRC_DIR)/main_window.cpp -o $(OBJ_DIR)/main_window.o
	$(CC)  $(CC_DEP_OPT) $(CCFLAGS) $(SRC_DIR)/main_window.cpp > $(OBJ_DIR)/main_window.d

$(OBJ_DIR)/main.o: $(SRC_DIR)/main.cpp
	$(CC)  $(CCFLAGS) $(SRC_DIR)/main.cpp -o $(OBJ_DIR)/main.o
	$(CC)  $(CC_DEP_OPT) $(CCFLAGS) $(SRC_DIR)/main.cpp > $(OBJ_DIR)/main.d

$(OBJ_DIR)/preferences.o: $(SRC_DIR)/preferences.cpp
	$(CC)  $(CCFLAGS) $(SRC_DIR)/preferences.cpp -o $(OBJ_DIR)/preferences.o
	$(CC)  $(CC_DEP_OPT) $(CCFLAGS) $(SRC_DIR)/preferences.cpp > $(OBJ_DIR)/preferences.d

$(OBJ_DIR)/select_points.o: $(SRC_DIR)/select_points.cpp
	$(CC)  $(CCFLAGS) $(SRC_DIR)/select_points.cpp -o $(OBJ_DIR)/select_points.o
	$(CC)  $(CC_DEP_OPT) $(CCFLAGS) $(SRC_DIR)/select_points.cpp > $(OBJ_DIR)/select_points.d
	
$(OBJ_DIR)/settings_dlg.o: $(SRC_DIR)/settings_dlg.cpp
	$(CC)  $(CCFLAGS) $(SRC_DIR)/settings_dlg.cpp -o $(OBJ_DIR)/settings_dlg.o
	$(CC)  $(CC_DEP_OPT) $(CCFLAGS) $(SRC_DIR)/settings_dlg.cpp > $(OBJ_DIR)/settings_dlg.d

$(OBJ_DIR)/utils.o: $(SRC_DIR)/utils.cpp
	$(CC)  $(CCFLAGS) $(SRC_DIR)/utils.cpp -o $(OBJ_DIR)/utils.o
	$(CC)  $(CC_DEP_OPT) $(CCFLAGS) $(SRC_DIR)/utils.cpp > $(OBJ_DIR)/utils.d

$(OBJ_DIR)/worker.o: $(SRC_DIR)/worker.cpp
	$(CC)  $(CCFLAGS) $(SRC_DIR)/worker.cpp -o $(OBJ_DIR)/worker.o
	$(CC)  $(CC_DEP_OPT) $(CCFLAGS) $(SRC_DIR)/worker.cpp > $(OBJ_DIR)/worker.d
