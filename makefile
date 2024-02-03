LUA_INC=-I /usr/include

all : cell.so

cell.so : cell.c
	gcc -Wall -g --shared -o $@ $^ $(LUA_INC)

clean :
	rm -f cell.so
