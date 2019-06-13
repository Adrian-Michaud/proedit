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
macro.o \
color_html.o

#DEBUG_FLAGS=-g
OPT_FLAGS= -O3

RGREP_OBJECTS=rgrep.o memory.o unix.o xinit.o clipboard.o

PE_OBJECTS= $(COMMON) unixd.o xinit.o clipboard.o

all: pe rgrep

pe : $(PE_OBJECTS)
	gcc -o $@ $(PE_OBJECTS) -D__GCC__ -L/usr/X11R6/lib -lX11 -lXext  

rgrep : $(RGREP_OBJECTS)
	gcc -o $@ $(RGREP_OBJECTS) -D__GCC__ -DRGREP -L/usr/X11R6/lib -lX11 -lXext   
      
%.o : ../%.c
	gcc -c $(DEBUG_FLAGS) $(OPT_FLAGS)  -D__GCC__  -Wall -W -Wredundant-decls -DX11_GUI $<

xinit.o:
	gcc -c $(DEBUG_FLAGS) $(OPT_FLAGS) -D__GCC__ -Wall -W -Wredundant-decls -DX11_GUI ../X11/xinit.c

unix.o:
	gcc -c $(DEBUG_FLAGS) $(OPT_FLAGS) -D__GCC__ -DRGREP -Wall -W -Wredundant-decls -DX11_GUI ../X11/unix.c

unixd.o:
	gcc -c $(DEBUG_FLAGS) $(OPT_FLAGS) -D__GCC__ -DOS_DAEMONIZE -Wall -W -Wredundant-decls -DX11_GUI ../X11/unix.c -o unixd.o

clipboard.o:
	gcc -c $(DEBUG_FLAGS) $(OPT_FLAGS) -D__GCC__ -Wall -W -Wredundant-decls -DX11_GUI ../X11/clipboard.c


clean:
	rm *.o
	rm pe
	rm rgrep
