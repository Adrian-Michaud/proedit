call clean.bat
rc proedit.rc
cl /Zi /DWIN32_GUI ..\utility.c ..\shell.c ..\spell.c ..\checkout.c ..\find.c ..\errors.c ..\match.c ..\bsd_cstyle.c ..\stubs.c ..\adrian_cstyle.c ..\proedit.c ..\wordwrap.c ..\indenting.c main_class.c display_class.c winmain.c win32_gui.c win32.c ..\file.c ..\display.c ..\block.c ..\clip.c ..\undo.c ..\input.c ..\cursor.c ..\edit.c ..\search.c ..\goto.c ..\merge.c ..\history.c ..\browse.c ..\calc.c ..\select.c ..\help.c ..\memory.c ..\config.c ..\picklist.c ..\operation.c ..\cstyle.c ..\tabs.c ..\hex.c ..\session.c ..\colorize.c ..\color_c.c ..\color_cs.c ..\color_html.c ..\sun_cstyle.c ..\bookmarks.c proedit.res user32.lib gdi32.lib shell32.lib comctl32.lib advapi32.lib /Fepe.exe
copy pe.exe "c:\Documents and Settings\Adrian\Desktop"
copy pe.exe "c:\windows"

