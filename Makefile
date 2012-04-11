#================================================
# User define parameter
#
PROGRAM = server_echo
CC = gcc
LINK = gcc
CFLAGS = -Wall 
#LDFLAGS = 

SRC_FILE_TYPE = c
SRC_DIR := src

LIBS := -lpthread
INCLUDE_PATH := -I$(SRC_DIR)/include 

#define main source file in which "main()" function exist!
#MAIN_FILE = main
MAIN_FILE = $(PROGRAM)

# module define which act as subdirectory reside in "src" directory
#SRC_MODULES = main conf ca comm pm utils ic reg timer 
SRC_MODULES = main 

#==================================================
# Optional User define parameter
#

#program build mode is: debug/release
BUILD_MODE := debug
#BUILD_MODE := release

#Auto dependent generated? (YES/NO)
#AUTO_DEPENDENT = YES
#AUTO_DEPENDENT = NO

#=================================================
#directory define
BUILD_DIR := ./bin
DEPS_DIR := $(BUILD_DIR)/depend

#================================================= 
#get out put directory
ifeq ($(BUILD_MODE),debug)
OUTPUT_DIR := $(BUILD_DIR)/debug
CFLAGS += -g -O0

else
ifeq ($(BUILD_MODE),release)
OUTPUT_DIR := $(BUILD_DIR)/release
CFLAGS += -O3

else
$(error "BUILD_MODE error!(release/debug)")
endif
endif

##set target
target = $(PROGRAM)

#add path
#VPATH = $(shell find $(SRC_DIR) -type d)
#VPATH = $(SRC_DIR)
VPATH += $(addprefix $(SRC_DIR)/, $(SRC_MODULES))

#FIND_SRC_FILES = $(shell find $(SRC_DIR) -maxdepth 1 -name "*.$(SRC_FILE_TYPE)")
#SRC_FILES = $(notdir $(FIND_SRC_FILES) )

FIND_SRC_FILES = $(foreach dir, $(VPATH), $(wildcard $(dir)/*.$(SRC_FILE_TYPE)))
SRC_FILES = $(notdir $(FIND_SRC_FILES) )

OBJ_FILES = $(SRC_FILES:.$(SRC_FILE_TYPE)=.o)

#auto depend file?
ifeq ($(AUTO_DEPENDENT) ,YES)
DEP_FILES = $(SRC_FILES:.$(SRC_FILE_TYPE)=.d)
endif

OUTPUT_OBJS = $(addprefix $(OUTPUT_DIR)/,$(OBJ_FILES))
OUTPUT_DEPS = $(addprefix $(DEPS_DIR)/,$(DEP_FILES))

#INCLUDE_PATH += $(addprefix -I , $(VPATH))

##======  start of init shell ======##
## exec init shell command

$(shell mkdir -p "$(OUTPUT_DIR)")
$(shell mkdir -p "$(DEPS_DIR)")
#$(foreach dir, $(SRC_MODULES),$(shell mkdir -p "$(DEPS_DIR)/$(dir)"))

##======= end of init ==============##

#add target
.PHONY:all
all: $(BUILD_DIR)/$(target)

.PHONY: help
help:
	@echo "make's target is:  "
	@echo "		make all      --  make all and generate target!"
	@echo "		make run      --  make all and run the target! "
	@echo "		make clean    --  make clean the object files!"
	@echo "		make cleanall --  make clean all ,clean everything!"
	@echo "Test:"
	@echo "		make test     --  make  test target!"
	@echo "		make testrun  --  make test target and run it!"
	@echo "		make help     --  this help!"
	@echo " "
	@echo " "



## 
#link all objs and libs
$(BUILD_DIR)/$(target): $(OUTPUT_DIR)/$(MAIN_FILE).o $(OUTPUT_DEPS) $(OUTPUT_OBJS) 
	$(LINK) $(LIB_PATH)  $< $(OUTPUT_OBJS) $(CFLAGS) -o $@ $(LIBS)
	@echo "Make '$(BUILD_DIR)/$(target)' finished!"
  
    
#compile source files into object files
$(OUTPUT_DIR)/%.o: %.$(SRC_FILE_TYPE) 
	$(CC) $(CFLAGS) $(INCLUDE_PATH) -I $(basename $<) -c $< -o $@

#src dir compiler
$(OUTPUT_DIR)/%.o: $(SRC_DIR)/%.$(SRC_FILE_TYPE)
	$(CC) $(CFLAGS) $(INCLUDE_PATH) -c $< -o $@

#
$(DEPS_DIR)/%.d: %.$(SRC_FILE_TYPE)
	@echo -n "$(OUTPUT_DIR)/" > $@
	$(CC) -MM $(INCLUDE_PATH) $< >> $@


.PHONY:test
test: $(OUTPUT_DIR)/test.o $(OUTPUT_DEPS) $(OUTPUT_OBJS)
	$(LINK) $(LIB_PATH) $(OUTPUT_OBJS) $(CFLAGS) $< -o $(BUILD_DIR)/$@ $(LIBS)
	

.PHONY:testrun
testrun: test
	$(BUILD_DIR)/test

.PHONY:run
run: $(BUILD_DIR)/$(target)
	$(BUILD_DIR)/$(target)

.PHONY:clean
clean:
	-rm -rf $(OUTPUT_DIR)
	-rm -f $(BUILD_DIR)/$(target)

.PHONY:cleanall
cleanall:clean
	-rm -rf $(BUILD_DIR)/*

#dep files
-include $(OUTPUT_DEPS)

#=========== user custom target define here ===================================
#.PHONY:client
client: $(OUTPUT_DIR)/client.o $(OUTPUT_DEPS) $(OUTPUT_OBJS)
	$(LINK) $(LIB_PATH) $(OUTPUT_OBJS) $(CFLAGS) $< -o $(BUILD_DIR)/$@ $(LIBS)

#.PHONY:test
fork: $(OUTPUT_DIR)/server_fork.o $(OUTPUT_DEPS) $(OUTPUT_OBJS)
	$(LINK) $(LIB_PATH) $(OUTPUT_OBJS) $(CFLAGS) $< -o $(BUILD_DIR)/$@ $(LIBS)

#.PHONY:test
prefork: $(OUTPUT_DIR)/server_prefork.o $(OUTPUT_DEPS) $(OUTPUT_OBJS)
	$(LINK) $(LIB_PATH) $(OUTPUT_OBJS) $(CFLAGS) $< -o $(BUILD_DIR)/$@ $(LIBS)

#.PHONY:test
thread: $(OUTPUT_DIR)/server_thread.o $(OUTPUT_DEPS) $(OUTPUT_OBJS)
	$(LINK) $(LIB_PATH) $(OUTPUT_OBJS) $(CFLAGS) $< -o $(BUILD_DIR)/$@ $(LIBS)

#.PHONY:test
prethread: $(OUTPUT_DIR)/server_prethread.o $(OUTPUT_DEPS) $(OUTPUT_OBJS)
	$(LINK) $(LIB_PATH) $(OUTPUT_OBJS) $(CFLAGS) $< -o $(BUILD_DIR)/$@ $(LIBS)

#.PHONY:test
mul: $(OUTPUT_DIR)/server_mul.o $(OUTPUT_DEPS) $(OUTPUT_OBJS)
	$(LINK) $(LIB_PATH) $(OUTPUT_OBJS) $(CFLAGS) $< -o $(BUILD_DIR)/$@ $(LIBS)
#.PHONY:test
epoll_lt: $(OUTPUT_DIR)/server_epoll_lt.o $(OUTPUT_DEPS) $(OUTPUT_OBJS)
	$(LINK) $(LIB_PATH) $(OUTPUT_OBJS) $(CFLAGS) $< -o $(BUILD_DIR)/$@ $(LIBS)
#.PHONY:test
epoll_et: $(OUTPUT_DIR)/server_epoll_et.o $(OUTPUT_DEPS) $(OUTPUT_OBJS)
	$(LINK) $(LIB_PATH) $(OUTPUT_OBJS) $(CFLAGS) $< -o $(BUILD_DIR)/$@ $(LIBS)
