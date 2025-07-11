package = "luadbi-duckdb"
version = "scm-0"

description = {
    summary = "Database abstraction layer",
    detailed = [[
        LuaDBI is a database interface library for Lua. It is designed
        to provide a RDBMS agnostic API for handling database
        operations. LuaDBI also provides support for prepared statement
        handles, placeholders and bind parameters for all database
        operations.

        This rock is the DuckDB DBD module. You will also need the
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
    DUCKDB = { header = "duckdb.h" }
}

build = {
    type = "builtin",
    modules = {
        ['dbd.duckdb'] = {
            sources = {
                'dbd/common.c',
                'dbd/duckdb/main.c',
                'dbd/duckdb/statement.c',
                'dbd/duckdb/connection.c'
            },

            libraries = {
                'duckdb'
            },

            incdirs = {
                "$(DUCKDB_INCDIR)",
                './'
            },

            libdirs = {
                "$(DUCKDB_LIBDIR)"
            }
        }
    }
}
