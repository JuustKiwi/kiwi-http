CC = gcc
CFLAGS = -std=gnu2x -Wall -Wextra -O0 -g -pthread
LDFLAGS = -pthread

SRCDIR = src
OBJDIR = .obj
INCDIR = include

SRC = $(wildcard $(SRCDIR)/*.c)
OBJ = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRC))
EXEC = kiwi-http

all: $(OBJDIR) $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(EXEC)
