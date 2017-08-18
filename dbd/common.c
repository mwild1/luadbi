#include <dbd/common.h>

const char *dbd_strlower(char *in) {
    char *s = in;

    while(*s) {
	*s= (*s <= 'Z' && *s >= 'A') ? (*s - 'A') + 'a' : *s;
	s++;
    }

    return in;
}

/*
 * replace '?' placeholders with {native_prefix}\d+ placeholders
 * to be compatible with native API
 */
char *dbd_replace_placeholders(lua_State *L, char native_prefix, const char *sql) {
    size_t len = strlen(sql);
    int num_placeholders = 0;
    int extra_space = 0;
    int i;
    char *newsql;
    int newpos = 1;
    int ph_num = 1;
    int in_quote = 0;
    char format_str[4];

    format_str[0] = native_prefix;
    format_str[1] = '%';
    format_str[2] = 'u';
    format_str[3] = '\0';

    /*
     * dumb count of all '?'
     * this will match more placeholders than necessesary
     * but it's safer to allocate more placeholders at the
     * cost of a few bytes than risk a buffer overflow
     */ 
    for (i = 1; i < len; i++) {
	if (sql[i] == '?') {
	    num_placeholders++;
	}
    }
    
    /*
     * this is MAX_PLACEHOLDER_SIZE-1 because the '?' is 
     * replaced with '{native_prefix}'
     */ 
    extra_space = num_placeholders * (MAX_PLACEHOLDER_SIZE-1); 

    /*
     * allocate a new string for the converted SQL statement
     */
    newsql = calloc(len+extra_space+1, sizeof(char));
    if(!newsql) {
    	lua_pushliteral(L, "out of memory");
	/* lua_error does not return. */
    	lua_error(L);
    }

    /* 
     * copy first char. In valid SQL this cannot be a placeholder
     */
    newsql[0] = sql[0];

    /* 
     * only replace '?' not in a single quoted string
     */
    for (i = 1; i < len; i++) {
	/*
	 * don't change the quote flag if the ''' is preceded 
	 * by a '\' to account for escaping
	 */
	if (sql[i] == '\'' && sql[i-1] != '\\') {
	    in_quote = !in_quote;
	}

	if (sql[i] == '?' && !in_quote) {
	    size_t n;

	    if (ph_num > MAX_PLACEHOLDERS) {
		luaL_error(L, "Sorry, you are using more than %d placeholders. Use %c{num} format instead", MAX_PLACEHOLDERS, native_prefix);
	    }

	    n = snprintf(&newsql[newpos], MAX_PLACEHOLDER_SIZE, format_str, ph_num++);

	    newpos += n;
	} else {
	    newsql[newpos] = sql[i];
	    newpos++;
	}
    }

    /* 
     * terminate string on the last position 
     */
    newsql[newpos] = '\0';

    /* fprintf(stderr, "[%s]\n", newsql); */
    return newsql;
}

void dbd_register(lua_State *L, const char *name,
		  const luaL_Reg *methods, const luaL_Reg *class_methods,
		  lua_CFunction gc, lua_CFunction tostring)
{
    /* Create a new metatable with the given name and then assign the methods
     * to it.  Set the __index, __gc and __tostring fields appropriately.
     */
    luaL_newmetatable(L, name);
#if LUA_VERSION_NUM < 502
    luaL_register(L, 0, methods);
#else
    luaL_setfuncs(L, methods, 0);
#endif
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, gc);
    lua_setfield(L, -2, "__gc");

    lua_pushcfunction(L, tostring);
    lua_setfield(L, -2, "__tostring");

    /* Create a new table and register the class methods with it */
    lua_newtable(L);
#if LUA_VERSION_NUM < 502
    luaL_register(L, 0, class_methods);
#else
    luaL_setfuncs(L, class_methods, 0);
#endif
}

