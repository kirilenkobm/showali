CC      := cc
CFLAGS  := -Wall -Wextra -std=c17
SRCDIR  := src
OBJDIR  := build
SRCS    := $(wildcard $(SRCDIR)/*.c)
OBJS    := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))
TARGET  := tseq

all: $(TARGET)

# Link executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile each .c into build/%.o
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Ensure build dir exists
$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)

.PHONY: all clean