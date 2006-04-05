What needed:
~~~~~~~~~~~~
- global scripting: if (): platform, game, any cvar
  comparision: float (<,>,<=,>=,!=,==) or string (ne,eq etc. -- see Perl)
- menu params (change defaults):
  - item spacing
  - auto-apply
- action (may be complex {}! and: may be cvar-"force-set" (cheat protect: only for system menus??)):
  - change cvar (for cvar fields)
  - exec command (with exit from menu or with nesting menus; may be -- only with nesting: use "menu_pop ; command" ?)
  - reload (reparse) menu (with different cvar configuration)
  - apply settings
  - exit from menu
- cvar types:
  - bool: yes/no, enabled/disabled, true/false, on/off
    + inverted boolean values (0 -- on, 1 -- off)
  - list (enum): repeat { const -- string }
  - int/float: ranged (spin: need range and step) or any (text field: need text length)
  - bitfield (dmflags etc.): cvar,mask -- name; one declaration per one bit (many per dword)
    use strtol with base=0 to get mask (this will allow to specify 0xHEX and decimal constants)
- callback (named procedures, may be registered): -- owner-draw / owner-behaviour fields
  - draw
  - change
  - get text (more simple, than "draw")
- static images (banners, logos, may be - with "GetImageName" callback) - specify image center or image upper-left corner
- static text (may be, with "GetText" callback)
- dividers (empty line, colorized text etc.)
- hints for current item (displayed in statusbar)
? inline submenus

FEAUTURE:
~~~~~~~~~
- file include
- tree-view menus (with sections and expand/collapse capabilities)
- combobox-style lists
- mouse GUI
- disabled fields
