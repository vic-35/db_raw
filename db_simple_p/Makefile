CC = gcc
LINK = $(CC)
STRIP = strip

SRC = ./
BUILD = ./objs
LIB = ./lib

CFLAGS      = -g -O2 $$(python3-config --includes --ldflags) -Wincompatible-pointer-types
CSAHREDLIBS = -shared

INCS = -I$(SRC) 

OBJS = $(BUILD)/main.o \

BINS = $(BUILD)/_db_simple_p.so

BINS2 = _db_simple_p.so 

all: prebuild \
	$(BINS)

$(BUILD)/_db_simple_p.so: \
	$(OBJS)
	$(CC) $(CFLAGS) $(CSAHREDLIBS) -o $(LIB)/_db_simple_p.so $(OBJS)
	$(STRIP) --strip-unneeded $(LIB)/_db_simple_p.so

$(BUILD)/main.o: $(DEPS) \
	$(SRC)/main.c
	$(CC) -c $(CFLAGS) -fPIC $(INCS) -o $(BUILD)/main.o $(SRC)/main.c

clean:
	rm -rf $(BUILD)
	rm -rf $(LIB)

prebuild:
	main -d $(BUILD) || mkdir -p $(BUILD)
	main -d $(LIB) || mkdir -p $(LIB)
	
install: 
	ldconfig
