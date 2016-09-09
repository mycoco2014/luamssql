#ifndef _TDS_SQLSERVER_
#define _TDS_SQLSERVER_

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"


//#include <sstream.h>
//#include <iostream.h>
//using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KEY_MAX_LENGTH		128
#define VAL_MAX_LENGTH		2048


#define LUASQL_ENVIRONMENT_MSSQL "MSSQL environment"
#define LUASQL_CONNECTION_MSSQL "MSSQL connection"
#define LUASQL_CURSOR_MSSQL "MSSQL cursor"

#ifndef LUASQL_API
#define LUASQL_API
#endif

#define MSSQL_SERVER_VERSION "XXXX.XXXX.XXX"

#define LUASQL_PREFIX "LuaSQL: "
#define LUASQL_TABLENAME "luasql"
#define LUASQL_ENVIRONMENT "Each driver must have an environment metatable"
#define LUASQL_CONNECTION "Each driver must have a connection metatable"
#define LUASQL_CURSOR "Each driver must have a cursor metatable"


#define MAX_ERROR_MSG 512



#endif
