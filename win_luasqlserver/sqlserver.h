#pragma once  

// 注意：一定要加上 extern "C"  
extern "C"  
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
};


#include <iostream> 
#include <sstream> 
using namespace std; 

#pragma comment( lib, "lua51.lib" )

#define KEY_MAX_LENGTH		128
#define VAL_MAX_LENGTH		2048

enum MyEnum
{
	TYPE_SQL_QUERY = 1,
	TYPE_SQL_UPDATE,
	TYPE_PROC_REC0RDSET,
	TYPE_PROC_NO_REC0RDSET,
};


// 0x01 
// 0x02 
// 0x03 
// 0x04 
// 0xF2
// 0xF3
// 0xF4

#if 0
#pragma comment(lib, "libeay32.lib")
#pragma comment(lib, "ssleay32.lib")
#endif

//#endif  


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

typedef struct {
	short      closed;
} env_data;

#define MAX_ERROR_MSG 512

using namespace std;

#import "c:\\program files\\common files\\system\\ado\\msado15.dll" no_namespace rename("EOF","adoEOF")
