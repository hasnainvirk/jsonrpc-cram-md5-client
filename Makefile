#Cross GCC compiler
CC = g++
SRCDIR = ./src
INCLDIR = ./include
CXXFLAGS = -c -Wall -I$(INCLDIR)

DEBUG = 0
TEST = 0

ifeq ($(DEBUG), 1)
CXXFLAGS += -g
endif

ifeq ($(TEST), 1)
CXXFLAGS += -D TEST
endif

INCLUDES := $(wildcard $(INCLDIR)/*.h)
SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJECTS  := $(SRCS:$(SRCDIR)/%.cpp=$(SRCDIR)/%.o)

LFLAGS   = -Wall -lm
LINKER = $(CC) -L $(INCLDIR) -o

rm       = rm -f

TARGET = ASSIGNMENT

all: $(TARGET)
	@echo "Assignment is compiled"

$(TARGET): $(OBJECTS)
	@$(LINKER) $@ $(LFLAGS) $(OBJECTS)
	@echo "Linking complete!"

$(OBJECTS): $(SRCDIR)/%.o : $(SRCDIR)/%.cpp 
	$(CC) $(CXXFLAGS) $< -o $@
	@echo "Compiled "$<" successfully!"
	
.PHONY:	clean 
clean:
	@$(rm) $(OBJECTS) *~ $(TARGET)

depend: $(SRCS)
	makedepend $(INCLUDES) $^

# DO NOT DELETE THIS LINE -- make depend needs it


