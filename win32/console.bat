call clean.bat
cl /Zi /DWIN32_CONSOLE ..\utility.c ..\spell.c ..\shell.c ..\checkout.c ..\find.c ..\errors.c ..\match.c ..\stubs.c ..\adrian_cstyle.c ..\wordwrap.c ..\indenting.c ..\bsd_cstyle.c ..\proedit.c win32_console.c win32.c ..\file.c ..\display.c ..\block.c ..\clip.c ..\undo.c ..\input.c ..\cursor.c ..\edit.c ..\search.c ..\goto.c ..\merge.c ..\history.c ..\browse.c ..\calc.c ..\select.c ..\help.c ..\memory.c ..\config.c ..\picklist.c ..\operation.c ..\cstyle.c ..\tabs.c ..\hex.c ..\session.c ..\colorize.c ..\color_c.c ..\color_v.c ..\color_cs.c ..\color_html.c ..\sun_cstyle.c ..\bookmarks.c ..\macro.c user32.lib advapi32.lib /Fepe.exe
@ren rem cl /Ox /DWIN32_CONSOLE ..\utility.c ..\spell.c ..\shell.c ..\checkout.c ..\find.c ..\errors.c ..\match.c ..\stubs.c ..\adrian_cstyle.c ..\wordwrap.c ..\indenting.c ..\bsd_cstyle.c ..\proedit.c win32_console.c win32.c ..\file.c ..\display.c ..\block.c ..\clip.c ..\undo.c ..\input.c ..\cursor.c ..\edit.c ..\search.c ..\goto.c ..\merge.c ..\history.c ..\browse.c ..\calc.c ..\select.c ..\help.c ..\memory.c ..\config.c ..\picklist.c ..\operation.c ..\cstyle.c ..\tabs.c ..\hex.c ..\session.c ..\colorize.c ..\color_c.c ..\color_v.c ..\color_cs.c ..\color_html.c ..\sun_cstyle.c ..\bookmarks.c user32.lib advapi32.lib /Fepe.exe
@rem copy pe.exe c:\windows
@rem cl /Ox /DWIN32_CONSOLE ..\rgrep.c ..\memory.c win32_console.c win32.c user32.lib advapi32.lib /Fergrep.exe
cl /Zi /DWIN32_CONSOLE ..\rgrep.c ..\memory.c win32_console.c win32.c user32.lib advapi32.lib /Fergrep.exe
@rem copy rgrep.exe c:\windows


