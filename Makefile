
OPENCL_PATH=/opt/AMDAPPSDK-3.0-0-Beta



CC=gcc
TEST_NAME=rtcore
LFLAGS= -g -Wall
INCS= -I $(OPENCL_PATH)/include
C_FILES := $(wildcard *.c)
OBJ_FILES := $(addprefix obj/, $(notdir $(C_FILES:.c=.o)))



all: $(TEST_NAME)


$(TEST_NAME): $(OBJ_FILES)
	$(CC) $(LFLAGS) $(OBJ_FILES) -O2 -lm -lOpenCL -lGL -lGLU -lglut -o $(TEST_NAME)



obj/%.o: %.c
	$(CC) -c -O2 $(CFLAGS) $(INCS) -o $@ $<


clean:
	rm -rf obj/*o *.brig $(TEST_NAME)











