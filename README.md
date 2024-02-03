# Text mode table cell

ImGui style api.

```lua
local cell = require "cell"

-- > Right alignment
-- = Center alignment
-- < Left alignment (default)
-- 40% : 40% of width
-- 3   : 3 char wdith
cell.open ">40% =3"
	cell.text "Hello"
	cell.text "|"
	cell.text "World"

	cell.nextrow()
	cell.row(2)

	cell.text "Very loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooog"
	cell.col(2)
	cell.text "Test"

	cell.nextrow()
	cell.open "50% >0"
		cell.text "Sub"
		cell.text "Table"
		cell.nextrow()
		cell.text "Line"
		cell.text "2"
	cell.close()

	cell.col(2)

	cell.open()
		cell.text "Alpha"
		cell.text "Beta"
		cell.text "Gramma"
		cell.nextrow()
		cell.text "One"
		cell.text "Two"
		cell.text "Three"
		cell.nextrow()
		cell.text "1"
		cell.text "2"
		cell.text "3"
	cell.close()

	cell.nextrow()

	cell.text "-------"
	cell.text "+"
	cell.text "-------"

cell.close()

local s = cell.frame(80, 25)
print(s)
```

> Output

```
                           Hello | World                                        
Very loooooooooooooooooooooooooo   Test                                         
oooooooooooooooooooooooooooooooo                                                
Sub                        Table   Alpha          Beta           Gramma         
Line                           2   One            Two            Three          
                                   1              2              3              
                         ------- + -------
```
