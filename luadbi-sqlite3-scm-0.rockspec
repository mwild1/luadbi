package = "luadbi-sqlite3"
version = "scm-0"

description = {
    summary = "Database abstraction layer",
    detailed = [[
        LuaDBI is a database interface library for Lua. It is designed
        to provide a RDBMS agnostic API for handling database
        operations. LuaDBI also provides support for prepared statement
        handles, placeholders and bind parameters for all database
        operations.

        This rock is the Sqlite3 DBD module. You will also need the
        base DBI module to use this software.
    ]],

    license = "MIT/X11",
    homepage = "https://github.com/mwild1/luadbi"
}

source = {
    url = "git+https://github.com/mwild1/luadbi.git",
}

dependencies = {
    "lua >= 5.1",
    "luadbi = scm"
}

external_dependencies = {
    SQLITE = { header = "sqlite3.h" }
}

build = {
    type = "builtin",
    modules = {
        ['dbd.sqlite3'] = {
            sources = {
                'dbd/common.c',
                'dbd/sqlite3/main.c',
                'dbd/sqlite3/statement.c',
                'dbd/sqlite3/connection.c'
            },

            libraries = {
                'sqlite3'
            },

            incdirs = {
                "$(SQLITE_INCDIR)",
                './'
            },

            libdirs = {
                "$(SQLITE_LIBDIR)"
            }
        }
    }
}