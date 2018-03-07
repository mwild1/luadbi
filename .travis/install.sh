#!/bin/bash
LUA=${LUA:-5.1}
DEPS="libmysqlclient-dev libsqlite3-dev postgresql-server-dev-all"

case ${LUA} in
	5.1)
		LUA_DEPS="lua5.1 liblua5.1-0-dev"
	;;
	5.[2-3])
		LUA_DEPS="lua${LUA} liblua${LUA}-dev"
	;;
esac

apt-get -qq update
apt-get install -y ${DEPS} ${LUA_DEPS}
