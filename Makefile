PROGRAM = client
CC = gcc
CFLAGS = -Wall 
LINK = gcc
#LDFLAGS = 

SRC_FILE_TYPE = c

#==================================================
# module define
SRC_MODULES = client 


#==================================================
#program build mode is: debug/release
BUILD_MODE := debug
#BUILD_MODE := release

#Auto dependent generated? (YES/NO)
#AUTO_DEPENDENT = YES
AUTO_DEPENDENT = NO

SRC_DIR = ./src
#==================================================
#add the lib you used here
#LIBS := -lLib1 -lLib2 -lLib3
LIBS := -lpthread
#LIBS := -lpthread -lxml2
#LIB_PATH := -Lpath1 -Lpath2 -Lpath3
LIB_PATH := 
INCLUDE_PATH := -I $(SRC_DIR) -I $(SRC_DIR)/include 
#INCLUDE_PATH := -I/usr/lib/XXX/include

include Makefile.base
