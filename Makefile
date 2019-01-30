EXECUTABLE = ./nil

CFLAGS = -Wall -g
LDFLAGS = 

OBJECTS = \
compiler.o \
closure.o \
symbol.o \
vector.o \
character.o \
environment.o \
pair.o \
number.o \
object.o \
main.o


%.o: %.c
	$(CC) $(CFLAGS) -c $<


$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LD_FLAGS) -o $(EXECUTABLE) $(OBJECTS)

all: $(EXECUTABLE)


.PHONY: check
check: $(EXECUTABLE)
	valgrind --leak-check=yes $(EXECUTABLE)

.PHONY: clean
clean:
	-rm $(EXECUTABLE)
	-rm $(OBJECTS)
