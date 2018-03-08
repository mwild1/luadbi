package = "luadbi-postgresql"
version = "scm-0"

description = {
    summary = "Database abstraction layer",
    detailed = [[
        LuaDBI is a database interface library for Lua. It is designed
        to provide a RDBMS agnostic API for handling database
        operations. LuaDBI also provides support for prepared statement
        handles, placeholders and bind parameters for all database
        operations.

        This rock is the PostgreSQL DBD module. You will also need the
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
    POSTGRES = { header = "postgresql/libpq-fe.h" }
}

build = {
    type = "builtin",
    modules = {
        ['dbd.postgresql'] = {
            sources = {
                'dbd/common.c',
                'dbd/postgresql/main.c',
                'dbd/postgresql/statement.c',
                'dbd/postgresql/connection.c'
            },

            libraries = {
                'pq'
            },

            incdirs = {
                "$(POSTGRES_INCDIR)/postgresql",
                './'
            },

            libdirs = {
                "$(POSTGRES_LIBDIR)"
            }
        }
    }
}