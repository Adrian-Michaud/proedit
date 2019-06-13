COMMON=proedit.o \
file.o \
shell.o \
utility.o \
spell.o \
errors.o \
checkout.o \
find.o \
wordwrap.o \
stubs.o \
display.o \
block.o \
clip.o \
undo.o \
input.o \
match.o \
cursor.o \
bookmarks.o \
edit.o \
indenting.o \
search.o \
goto.o \
merge.o \
history.o \
browse.o \
calc.o \
select.o \
help.o \
memory.o \
config.o \
picklist.o \
operation.o \
cstyle.o \
bsd_cstyle.o \
sun_cstyle.o \
adrian_cstyle.o \
tabs.o \
hex.o \
session.o \
colorize.o \
color_c.o \
color_v.o \
color_cs.o \
color_html.o

#DEBUG_FLAGS=-g
OPT_FLAGS= -O3

RGREP_OBJECTS=rgrep.o memory.o unix.o screen.o

#OBJECTS= $(COMMON) unixd.o
OBJECTS= $(COMMON) unix.o screen.o

#all: pe2 rgrep
all: pe2

pe2 : $(OBJECTS)
	gcc -o $@ $(OBJECTS) -D__GCC__ -lncurses

rgrep2 : $(RGREP_OBJECTS)
	gcc -o $@ $(RGREP_OBJECTS) -D__GCC__ -lncurses
      
%.o : ../%.c
	gcc -c $(DEBUG_FLAGS) $(OPT_FLAGS)  -D__GCC__  -Wall -W -Wredundant-decls -DNCURSES $<

unix.o: unix.c
	gcc -c $(DEBUG_FLAGS) $(OPT_FLAGS) -D__GCC__ -Wall -W -Wredundant-decls -DNCURSES unix.c

screen.o: screen.c
	gcc -c $(DEBUG_FLAGS) $(OPT_FLAGS) -D__GCC__ -Wall -W -Wredundant-decls -DNCURSES screen.c

#unixd.o:
#	gcc -c $(DEBUG_FLAGS) $(OPT_FLAGS) -D__GCC__ -DOS_DAEMONIZE -Wall -W -Wredundant-decls -DNCURSES unix.c -o unixd.o

clean:
	rm *.o
	rm pe
	rm rgrep
