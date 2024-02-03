#define LUA_MOD
#include <lua.h>
#include <lauxlib.h>
#include <string.h>

#define MAX_ROWS 32
#define MAX_COLS 32

struct table_cell {
	int id;
};

struct table_row {
	int height;
};

struct table_col {
	int alignment;
	int width;
};

struct table {
	int rows;
	int cols;
	int x;	// current col
	int width;
	int height;
	struct table_row row[MAX_ROWS];
	struct table_col layout[MAX_COLS];
	struct table_cell c[MAX_ROWS][MAX_COLS];
};

static void
table_init(struct table *t) {
	memset(t, 0, sizeof(*t));
	t->rows = 1;
	t->cols = 1;
}

static void
table_nextrow(struct table *t) {
	t->x = 0;
	t->rows++;
	if (t->rows < MAX_COLS) {
		t->row[t->rows].height = 0;
	}
}

static void
table_rowset(struct table *t, int height) {
	int row = t->rows -1;
	if (t->rows < MAX_ROWS) {
		t->row[row].height = height;
	}
}

static void
table_insert(struct table *t, int id) {
	if (t->cols >= MAX_COLS)
		return;
	t->x++;
	if (t->x > MAX_ROWS)
		return;
	if (t->x > t->cols)
		t->cols = t->x;
	t->c[t->rows-1][t->x-1].id = id;
};

static void
table_col(struct table *t, int col) {
	t->x = col;
	if (t->x >= MAX_ROWS)
		return;
	if (t->x > t->cols)
		t->cols = t->x;
}

static int
ltable_nextrow(lua_State *L) {
	struct table * t = (struct table *)lua_touserdata(L, 1);
	table_nextrow(t);
	return 0;
}

static int
ltable_rowset(lua_State *L) {
	struct table * t = (struct table *)lua_touserdata(L, 1);
	int value = luaL_checkinteger(L, 2);
	table_rowset(t, value);
	return 0;
}

static int
ltable_insert(lua_State *L) {
	struct table * t = (struct table *)lua_touserdata(L, 1);
	int value = luaL_checkinteger(L, 2);
	lua_pushinteger(L, t->x);
	lua_pushinteger(L, t->rows - 1);
	table_insert(t, value);
	return 2;
}

static int
ltable_col(lua_State *L) {
	struct table * t = (struct table *)lua_touserdata(L, 1);
	int value = luaL_checkinteger(L, 2);
	table_col(t, value);
	return 0;
}

static int
ltable_new(lua_State *L) {
	struct table *t = lua_newuserdatauv(L, sizeof(*t), 1);
	table_init(t);
	return 1;
}

static int
ltable_clear(lua_State *L) {
	struct table * t = (struct table *)lua_touserdata(L, 1);
	table_init(t);
	return 0;
}

static int
calc_layout(struct table_col layout[], const struct table_col *fmt, int n, int width, int cols) {
	int flex = 0;
	int i;
	for (i = 0; i < n ; i++) {
		int f = fmt[i].width;
		if (f < 0) {
			f = width * (-f) / 100;
			if (f <= 0)
				f = 1;
		} else if (f == 0) {
			flex++;
		}
		layout[i].alignment = fmt[i].alignment;
		layout[i].width = f;
	}
	if (cols > n) {
		flex += cols - n;
		for (i = n; i < cols;i++) {
			layout[i].alignment = -1;
			layout[i].width = 0;
		}
	}
	int avg_width = 0;
	if (flex > 0) {
		avg_width = width / flex;
		if (avg_width <= 0)
			avg_width = 1;
	}
	int w = 0;
	for (i = 0; i < cols; i++) {
		int cell_w = layout[i].width;
		if (cell_w == 0)
			cell_w = avg_width;
		if (cell_w + w > width)
			cell_w = width - w;
		layout[i].width = cell_w;
		w += cell_w;
	}
	return w;
}

static int
format_row(lua_State *L, char *buf, struct table *t, int line, const struct table_col *layout, int max_row, int width) {
	if (max_row <= 0)
		return 0;
	int limit_height = t->row[line].height;
	if (limit_height == 0) {
		limit_height = max_row;
	} else if (limit_height > max_row) {
		limit_height = max_row;
	}
	struct table_cell *cell = t->c[line];
	int i;
	int offset = 0;
	int max_height = 1;
	for (i=0;i<t->cols;i++) {
		int id = cell[i].id;
		int cell_width = layout[i].width;
		int alignment = layout[i].alignment;
		if (id > 0) {
			int limit = limit_height;
			lua_geti(L, 2, id);
			size_t sz;
			const char * text = luaL_tolstring(L, -1, &sz);
			lua_pop(L, 1);
			char * line = buf;
			while (sz > cell_width && limit > 0) { 
				// multi-line
				memcpy(line + offset, text, cell_width);
				line += width + 1;
				--limit;
				sz -= cell_width;
				text += cell_width;
			}
			if (limit > 0) {
				if (alignment < 0) {
					// left
					memcpy(line + offset, text, sz);
				} else if (alignment == 0) {
					// center
					int spacing = (cell_width - sz) / 2;
					memcpy(line + offset + spacing, text, sz);
				} else {
					// right
					int spacing = (cell_width - sz);
					memcpy(line + offset + spacing, text, sz);
				}
				limit--;
			}
			int height = limit_height - limit;
			if (height > max_height)
				max_height = height;
		} else if (id < 0) {
			// inner table
			if (lua_geti(L, 2, id) != LUA_TUSERDATA)
				return luaL_error(L, "Invalid inner table");
			struct table *inner = (struct table *)lua_touserdata(L, -1);
			lua_getiuservalue(L, -1, 1);
			const char *inner_buf = (const char *)lua_touserdata(L, -1);
			lua_pop(L, 2);
			int lines = inner->height;
			int inner_width = inner->width;
			if (inner_width > cell_width)
				inner_width = cell_width;
			if (lines > limit_height)
				lines = limit_height;
			int j;
			char * line = buf;
			for (j = 0; j < lines; j++) {
				memcpy(line + offset, inner_buf, inner_width);
				line += width + 1;
				inner_buf += inner->width + 1;
			}
			if (lines > max_height)
				max_height = lines;
		}
		offset += cell_width;
	}
	return max_height;
}

static char *
alloc_buf(lua_State *L, size_t needsize) {
	char * buf = lua_touserdata(L, -1);
	lua_pop(L, 1);
	if (buf) {
		size_t len = lua_rawlen(L, -1);
		if (len >= needsize)
			return buf;
	}
	buf = lua_newuserdatauv(L, needsize, 0);
	lua_setiuservalue(L, 1, 1);
	return buf;
}

static int
ltable_setlayout(lua_State *L) {
	struct table * t = (struct table *)lua_touserdata(L, 1);
	size_t sz;
	const char * fmt = luaL_checklstring(L, 2, &sz);
	int max_width = luaL_checkinteger(L, 3);
	const struct table_col *col = (const struct table_col *)fmt;
	int n = sz / sizeof(struct table_col);
	int width = calc_layout(t->layout, col, n, max_width, t->cols);
	lua_pushinteger(L, width);
	return 1;
}

static int
ltable_format(lua_State *L) {
	struct table * t = (struct table *)lua_touserdata(L, 1);
	luaL_checktype(L, 2, LUA_TTABLE);	// text map
	int width = luaL_checkinteger(L, 3);
	int height = luaL_checkinteger(L, 4);

	size_t needsize = (width + 1) * height;

	lua_getiuservalue(L, 1, 1);
	char * buf = alloc_buf(L, needsize);
	memset(buf, 0x20, needsize);

	char * ptr = buf;

	int i;
	int lines = 0;
	for (i = 0; i < t->rows; i++) {
		int row = format_row(L, ptr, t, i, t->layout, height, width);
		if (row == 0)
			break;
		lines += row;
		height -= row;
		ptr += row * (width + 1);		
	}
	for (i = 0; i < lines; i++) {
		buf[i * (width + 1) + width] = '\n';
	}
	t->width = width;
	t->height = lines;
	return 0;
}

static int
ltable_tostring(lua_State *L) {
	struct table * t = (struct table *)lua_touserdata(L, 1);
	lua_getiuservalue(L, 1, 1);
	const char * buf = (const char *)lua_touserdata(L, -1);
	if (buf == NULL)
		return luaL_error(L, "Invalid buffer");
	lua_pushlstring(L, buf, t->height * (t->width + 1));
	return 1;
}

static int
ltable_colwidth(lua_State *L) {
	struct table * t = (struct table *)lua_touserdata(L, 1);
	int col = luaL_checkinteger(L, 2);
	if (col >= t->cols)
		return luaL_error(L, "Invalid col %d", col);
	lua_pushinteger(L, t->layout[col].width);
	return 1;
}

static inline char
alignment_char(int a) {
	if (a < 0)
		return '<';
	else if (a == 0)
		return '=';
	else
		return '>';
}

static int
ltable_layout(lua_State *L) {
	struct table * t = (struct table *)lua_touserdata(L, 1);
	luaL_Buffer b;
	luaL_buffinit(L, &b);
	int i;
	for (i=0;i<t->cols;i++) {
		lua_pushfstring(L, "%c%d", alignment_char(t->layout[i].alignment), t->layout[i].width);
		luaL_addvalue(&b);
	}
	luaL_pushresult(&b);
	return 1;
}

LUAMOD_API int
luaopen_cell_core(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "new", ltable_new },
		{ "clear", ltable_clear },
		{ "nextrow", ltable_nextrow },
		{ "rowset", ltable_rowset },
		{ "insert", ltable_insert },
		{ "col", ltable_col },
		{ "setlayout", ltable_setlayout },
		{ "layout", ltable_layout },
		{ "format", ltable_format },
		{ "colwidth", ltable_colwidth },
		{ "tostring", ltable_tostring },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	return 1;
}
