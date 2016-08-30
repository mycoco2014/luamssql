#include "sqlserver.h"
#include <windows.h>


#define RF(funcName) { #funcName, funcName }  

//
////////////////////////////////////////////////////////////////////////////////////////  
//static const char hexdigits[] = {  
//	'0', '1', '2', '3', '4', '5', '6', '7',  
//	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'  
//};  
//
////////////////////////////////////////////////////////////////////////////////////////  
//int StrToHex(const char *buffer,int len,char **new_buffer) {  
//	char *out = new char[2*len+3];  
//	char *po = out;  
//	strncpy(po,"0x",2);  
//	po += 2;  
//	const char *p = buffer;  
//	char ch = *p;  
//	int k = 0;  
//	while(k<len) {  
//		*po = hexdigits[(ch>>4)&0x0F];  
//		po++;  
//		*po = hexdigits[ch&0x0F];  
//		po++;  
//		ch = *++p;  
//		k++;  
//	}  
//	*po = '\0';  
//
//	*new_buffer = out;  
//
//	return 0;  
//}
//
////////////////////////////////////////////////////////////////////////////////////////  
//int CvtCharArrayValue(VARIANT val,char **out) {  
//	if (val.vt!=(VT_ARRAY|VT_UI1))  
//		return -1;  
//	SAFEARRAY *psa = val.parray; 
//	printf("---psa->cDims %d \r\n", psa->cDims);
//	if (psa->cDims!=1) 
//		return -1;  
//
//	HRESULT hr;  
//	LONG lLbound1,lUbound1;  
//	hr = SafeArrayGetLBound(psa, 1, &lLbound1);  
//	if(FAILED(hr))  
//		return -1;  
//	hr = SafeArrayGetUBound(psa, 1, &lUbound1);  
//	if(FAILED(hr))  
//		return -1;  
//
//	int size = lUbound1-lLbound1+1;  
//	DATE *buffer  = new DATE[size];
//	for (int i=lLbound1;i<=lUbound1;i++) {  
//		long ix = i;  
//		
//		HRESULT hr = SafeArrayGetElement(psa, &ix, &buffer[i]);  
//		if(FAILED(hr)) {
//			printf("---SafeArrayGetElement---failed!\r\n");
//			delete []buffer;  
//			return -1;
//		}  
//	}
//
//	printf("---SafeArrayGetElement---succed!!\r\n");
//	
//
//	return 0;  
//}  

char *w2cstatic(char*pcstr, const wchar_t *pwstr)
{
	int nlength=(int)wcslen(pwstr);
	memset(pcstr,0, sizeof(pcstr));
	//获取转换后的长度
	int nbytes = WideCharToMultiByte( 0, // specify the code page used to perform the conversion
		0,         // no special flags to handle unmapped characters
		pwstr,     // wide character string to convert
		nlength,   // the number of wide characters in that string
		NULL,      // no output buffer given, we just want to know how long it needs to be
		0,
		NULL,      // no replacement character given
		NULL );    // we don't want to know if a character didn't make it through the translation
	// make sure the buffer is big enough for this, making it larger if necessary

	// 通过以上得到的结果，转换unicode 字符为ascii 字符
	WideCharToMultiByte( 0, // specify the code page used to perform the conversion
		0,         // no special flags to handle unmapped characters
		pwstr,   // wide character string to convert
		nlength,   // the number of wide characters in that string
		pcstr, // put the output ascii characters at the end of the buffer
		nbytes,                           // there is at least this much space there
		NULL,      // no replacement character given
		NULL );
	return pcstr ;
}


char* w2c(char *m_char,const WCHAR* wp)  
{  
	int len= WideCharToMultiByte(CP_ACP,0,wp,wcslen(wp),NULL,0,NULL,NULL);
	if(m_char == NULL)
	{
		m_char = new char[len+1];
	}
	WideCharToMultiByte(CP_ACP,0,wp,wcslen(wp),m_char,len,NULL,NULL);
	m_char[len]='\0';
	return m_char;
} 

WCHAR *c2w(const char* buffer)
{
	size_t len = strlen(buffer);
	size_t wlen = MultiByteToWideChar(CP_ACP, 0, (const char*)buffer, int(len), NULL, 0);
	WCHAR *wBuf = new WCHAR[wlen + 1];
	memset(wBuf,0,wlen + 1);
	MultiByteToWideChar(CP_ACP, 0, (const char*)buffer, int(len), wBuf, int(wlen));
	wBuf[wlen] = 0x00;
	return wBuf;
}

WCHAR *c2wStatic(const char* buffer,WCHAR *wBuf)
{
	size_t len = strlen(buffer);
	size_t wlen = MultiByteToWideChar(CP_ACP, 0, (const char*)buffer, int(len), NULL, 0);
	memset(wBuf,0,sizeof(wBuf));
	MultiByteToWideChar(CP_ACP, 0, (const char*)buffer, int(len), wBuf, int(wlen));
	wBuf[wlen] = 0x00;
	return wBuf;
}

static int env_gc (lua_State *L) {
	env_data *env= (env_data *)luaL_checkudata (L, 1, LUASQL_ENVIRONMENT_MSSQL);
	if (env != NULL && !(env->closed)){
		env->closed = 1;
	}
	return 0;
}
static int env_close (lua_State *L) {
	env_data *env= (env_data *)luaL_checkudata (L, 1, LUASQL_ENVIRONMENT_MSSQL);
	luaL_argcheck (L, env != NULL, 1, LUASQL_PREFIX"environment expected");
	if (env->closed) {
		lua_pushboolean (L, 0);
		return 1;
	}
	env->closed = 1;
	lua_pushboolean (L, 1);
	return 1;
}


typedef struct {
	short				closed;
	int					env;                /* reference to environment */
	_ConnectionPtr     *ms_conn;
} conn_data;

typedef struct {
	short      closed;
	int        conn;               /* reference to connection */
	int        numcols;            /* number of columns */
	int        colnames, coltypes; /* reference to column information tables */
	_RecordsetPtr *ms_res;
} cur_data;

LUASQL_API void luasql_setmeta (lua_State *L, const char *name) {
	luaL_getmetatable (L, name);
	lua_setmetatable (L, -2);
}


static int create_environment (lua_State *L) {
	env_data *env = (env_data *)lua_newuserdata(L, sizeof(env_data));
	luasql_setmeta (L, LUASQL_ENVIRONMENT_MSSQL);
	/* fill in structure */
	env->closed = 0;
	return 1;
}



typedef struct { short  closed; } pseudo_data;

/*
** Return the name of the object's metatable.
** This function is used by `tostring'.
*/
static int luasql_tostring (lua_State *L) {
	char buff[100];
	pseudo_data *obj = (pseudo_data *)lua_touserdata (L, 1);
	if (obj->closed)
		strcpy (buff, "closed");
	else
		sprintf (buff, "%p", (void *)obj);
	lua_pushfstring (L, "%s (%s)", lua_tostring(L,lua_upvalueindex(1)), buff);
	return 1;
}


#if !defined LUA_VERSION_NUM || LUA_VERSION_NUM==501
/*
** Adapted from Lua 5.2.0
*/
void luaL_setfuncs (lua_State *L, const luaL_Reg *l, int nup) {
	luaL_checkstack(L, nup+1, "too many upvalues");
	for (; l->name != NULL; l++) {	/* fill the table with given functions */
		int i;
		lua_pushstring(L, l->name);
		for (i = 0; i < nup; i++)	/* copy upvalues to the top */
			lua_pushvalue(L, -(nup + 1));
		lua_pushcclosure(L, l->func, nup);	/* closure with those upvalues */
		lua_settable(L, -(nup + 3));
	}
	lua_pop(L, nup);	/* remove upvalues */
}
#endif

/*
** Create a metatable and leave it on top of the stack.
*/
LUASQL_API int luasql_createmeta (lua_State *L, const char *name, const luaL_Reg *methods) {
	if (!luaL_newmetatable (L, name))
		return 0;

	/* define methods */
	luaL_setfuncs (L, methods, 0);

	/* define metamethods */
	lua_pushliteral (L, "__index");
	lua_pushvalue (L, -2);
	lua_settable (L, -3);

	lua_pushliteral (L, "__tostring");
	lua_pushstring (L, name);
	lua_pushcclosure (L, luasql_tostring, 1);
	lua_settable (L, -3);

	lua_pushliteral (L, "__metatable");
	lua_pushliteral (L, LUASQL_PREFIX"you're not allowed to get this metatable");
	lua_settable (L, -3);

	return 1;
}


/*
** Assumes the table is on top of the stack.
*/
LUASQL_API void luasql_set_info (lua_State *L) {
	lua_pushliteral (L, "_COPYRIGHT");
	lua_pushliteral (L, "Copyright (C) xxxxx");
	lua_settable (L, -3);
	lua_pushliteral (L, "_DESCRIPTION");
	lua_pushliteral (L, "LuaSQL is a simple interface from Lua to a DBMS");
	lua_settable (L, -3);
	lua_pushliteral (L, "_VERSION");
	lua_pushliteral (L, "LuaSQL 2.3.1 x");
	lua_settable (L, -3);
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


/*
** Check for valid environment.
*/
static env_data *getenvironment (lua_State *L) {
	env_data *env = (env_data *)luaL_checkudata (L, 1, LUASQL_ENVIRONMENT_MSSQL);
	luaL_argcheck (L, env != NULL, 1, "environment expected");
	luaL_argcheck (L, !env->closed, 1, "environment is closed");
	return env;
}


/*
** Create a new Connection object and push it on top of the stack.
*/
static int create_connection (lua_State *L, int env, _ConnectionPtr *const ms_conn) {
	conn_data *conn = (conn_data *)lua_newuserdata(L, sizeof(conn_data));
	luasql_setmeta (L, LUASQL_CONNECTION_MSSQL);

	/* fill in structure */
	conn->closed = 0;
	conn->env = LUA_NOREF;
	conn->ms_conn = ms_conn;
	lua_pushvalue (L, env);
	conn->env = luaL_ref (L, LUA_REGISTRYINDEX);
	return 1;
}

/*
** Connects to a data source.
**     param: one string for each connection parameter, said
**     datasource, username, password, host and port.
*/
static int env_connect (lua_State *L) {
	
	const char *db_constr = luaL_checkstring(L,2);
	unsigned long timeout = (unsigned long)luaL_checkinteger(L,3);
	WCHAR  *pConStr = c2w(db_constr);

	_ConnectionPtr *conn = new _ConnectionPtr;

	getenvironment(L); /* validade environment */

	HRESULT hr ;

	SCODE sc = ::OleInitialize(NULL);
	if (FAILED(sc))
	{
		lua_pushboolean(L,FALSE);
		lua_pushfstring(L,"OleInitialize failed,error code:%d ",GetLastError());
		goto ExitHandler;
	}

	hr = conn->CreateInstance(__uuidof(Connection));
	if(FAILED(hr))
	{
		lua_pushnil(L);
		lua_pushfstring(L,"CreateInstance failed,error code:%d ",GetLastError());
		return 2;
	}
	if(timeout > 0 ) 
	{
		(*conn)->put_ConnectionTimeout(long(timeout));
	}

	try
	{
		hr = (*conn)->Open((_bstr_t)pConStr,"","",adModeUnknown);
		if(FAILED(hr)) 
		{
			lua_pushnil(L);
			lua_pushfstring(L,"Open failed,error code:%d ",GetLastError());
			goto ExitHandler;
		}
	}
	catch(_com_error ex)
	{
		_bstr_t bstrDescription(ex.Description());
		char errDesc[MAX_ERROR_MSG]={0};
		w2cstatic(errDesc,bstrDescription);

		_bstr_t bstrSource(ex.Source());
		char errSource[MAX_ERROR_MSG] = {0};
		w2cstatic(errSource,bstrSource);

		lua_pushnil(L);
		lua_pushfstring(L,"%s %s",errSource,errDesc);

		goto ExitHandler;
	}

	delete []pConStr;
	pConStr = NULL;
	return create_connection(L, 1, conn);

ExitHandler:
	delete []pConStr;
	pConStr = NULL;
	return 2;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


static int conn_gc (lua_State *L) {
	conn_data *conn=(conn_data *)luaL_checkudata(L, 1, LUASQL_CONNECTION_MSSQL);
	if (conn != NULL && !(conn->closed)) {
		/* Nullify structure fields. */
		conn->closed = 1;
		luaL_unref (L, LUA_REGISTRYINDEX, conn->env);
		//mysql_close (conn->my_conn);
		if((*(conn->ms_conn))->State){
			(*(conn->ms_conn))->Close();
		}
		delete conn->ms_conn ;
		conn->ms_conn = NULL;
	}
	return 0;
}

/*
** Close a Connection object.
*/
static int conn_close (lua_State *L) {
	conn_data *conn=(conn_data *)luaL_checkudata(L, 1, LUASQL_CONNECTION_MSSQL);
	luaL_argcheck (L, conn != NULL, 1, LUASQL_PREFIX"connection expected");
	if (conn->closed) {
		lua_pushboolean (L, 0);
		return 1;
	}
	conn_gc(L);
	lua_pushboolean (L, 1);
	return 1;
}

//////////////////////////////////////////////////////////////////////////


/*
** Check for valid connection.
*/
static conn_data *getconnection (lua_State *L) {
	conn_data *conn = (conn_data *)luaL_checkudata (L, 1, LUASQL_CONNECTION_MSSQL);
	luaL_argcheck (L, conn != NULL, 1, "connection expected");
	luaL_argcheck (L, !conn->closed, 1, "connection is closed");
	return conn;
}

/*
** Commit the current transaction.
*/
static int conn_commit (lua_State *L) {
	conn_data *conn = getconnection (L);
	HRESULT hr ;
	try
	{
		hr = (*(conn->ms_conn))->CommitTrans();
		if(FAILED(hr)) {
			lua_pushboolean(L, FALSE);
			lua_pushstring(L,"CommitTrans Failed!");
		} else {
			lua_pushboolean(L, TRUE);
			lua_pushstring(L,"");
		}
	}
	catch(_com_error ex)
	{
		_bstr_t bstrDescription(ex.Description());
		char errDesc[MAX_ERROR_MSG]={0};
		w2cstatic(errDesc,bstrDescription);
		_bstr_t bstrSource(ex.Source());
		char errSource[MAX_ERROR_MSG] = {0};
		w2cstatic(errSource,bstrSource);
		lua_pushnil(L);
		lua_pushfstring(L,"%s %s",errSource,errDesc);
	}
	return 2;
}

/*
** Rollback the current transaction.
*/
static int conn_rollback (lua_State *L) {
	conn_data *conn = getconnection (L);
	try
	{
		HRESULT hr ;
		hr = (*(conn->ms_conn))->RollbackTrans();
		if(FAILED(hr)) {
			lua_pushboolean(L, FALSE);
			lua_pushstring(L,"RollbackTrans Failed!");
		} else {
			lua_pushboolean(L, TRUE);
			lua_pushstring(L,"");
		}
	}
	catch(_com_error ex)
	{
		_bstr_t bstrDescription(ex.Description());
		char errDesc[MAX_ERROR_MSG]={0};
		w2cstatic(errDesc,bstrDescription);
		_bstr_t bstrSource(ex.Source());
		char errSource[MAX_ERROR_MSG] = {0};
		w2cstatic(errSource,bstrSource);
		lua_pushnil(L);
		lua_pushfstring(L,"%s %s",errSource,errDesc);
	}
	return 2;
}

/*
** Set "auto commit" property of the connection. Modes ON/OFF
** BeginTrans
*/
static int conn_begintrans (lua_State *L) {
	conn_data *conn = getconnection (L);
	try
	{
		long ret = (*(conn->ms_conn))->BeginTrans();
		lua_pushnumber(L,ret);
		lua_pushstring(L,"");
	}
	catch(_com_error ex)
	{
		_bstr_t bstrDescription(ex.Description());
		char errDesc[MAX_ERROR_MSG]={0};
		w2cstatic(errDesc,bstrDescription);
		_bstr_t bstrSource(ex.Source());
		char errSource[MAX_ERROR_MSG] = {0};
		w2cstatic(errSource,bstrSource);
		lua_pushnil(L);
		lua_pushfstring(L,"%s %s",errSource,errDesc);
	}
	return 2;
}

/*
** Create a new Cursor object and push it on top of the stack.
*/
static int create_cursor (lua_State *L, int conn, _RecordsetPtr *result, int cols) {
	cur_data *cur = (cur_data *)lua_newuserdata(L, sizeof(cur_data));
	luasql_setmeta (L, LUASQL_CURSOR_MSSQL);

	/* fill in structure */
	cur->closed = 0;
	cur->conn = LUA_NOREF;
	cur->numcols = cols;
	cur->colnames = LUA_NOREF;
	cur->coltypes = LUA_NOREF;
	cur->ms_res = result;
	lua_pushvalue (L, conn);
	cur->conn = luaL_ref (L, LUA_REGISTRYINDEX);

	return 1;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
unsigned long mssql_field_count(_RecordsetPtr *pRecordset){
	FieldsPtr m_Fields = (*pRecordset)->GetFields();
	return m_Fields->Count;
}


// 只返回行数
static int conn_update(lua_State *L){
	conn_data *conn = getconnection (L);
	const char *db_querysql = luaL_checkstring(L,2);
	if(db_querysql == NULL ) {
		lua_pushnil(L);
		lua_pushstring(L,"args is error");
		lua_pushnil(L);
		return 3;
	}
	if(adStateOpen != (*(conn->ms_conn))->State){
		lua_pushnil(L);
		lua_pushstring(L,"connect is closed");
		lua_pushnil(L);
		return 3;
	}

	WCHAR *wupdatesql = c2w(db_querysql);
	try{

		_variant_t RecordsAffected;

		_CommandPtr    pCommand;
		pCommand.CreateInstance(__uuidof(Command)); 

		pCommand->ActiveConnection = (*(conn->ms_conn)); 
		pCommand->CommandText = (_bstr_t)wupdatesql;
		pCommand->CommandType=adCmdText; 
		pCommand->Parameters->Refresh(); 
		pCommand->Execute(NULL,&RecordsAffected,adCmdUnknown);

		lua_pushnumber(L,RecordsAffected.intVal);
		lua_pushstring(L,"");
		lua_pushnil(L);
	}
	catch(_com_error ex)
	{
		// ERROR
		// https://msdn.microsoft.com/en-us/library/ms678037(v=vs.85).aspx
		_bstr_t bstrDescription(ex.Description());
		char errDesc[MAX_ERROR_MSG]={0};
		w2cstatic(errDesc,bstrDescription);
		_bstr_t bstrSource(ex.Source());
		char errSource[MAX_ERROR_MSG] = {0};
		w2cstatic(errSource,bstrSource);
		lua_pushnil(L);
		lua_pushfstring(L,"%s %s",errSource,errDesc);
		lua_pushstring(L,"Com Error Exception");
		goto ExitHandler;
	}
ExitHandler:
	delete []wupdatesql;
	wupdatesql = NULL;
	return 3;
}

// 1 RecordSet
// 2 RecordsAffected
// 3 error string
// 4 true is Exception
static int conn_execute(lua_State *L){
	conn_data *conn = getconnection (L);
	const char *db_querysql = luaL_checkstring(L,2);
	if(db_querysql == NULL ) {
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushstring(L,"args is error");
		lua_pushnil(L);
		return 4;
	}
	if(adStateOpen != (*(conn->ms_conn))->State){
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushstring(L,"connect is closed");
		lua_pushnil(L);
		return 4;
	}

	WCHAR *wexecutesql = c2w(db_querysql);
	try{
		_RecordsetPtr  *pRecordset = new _RecordsetPtr;
		pRecordset->CreateInstance(__uuidof(Recordset));
		_variant_t RecordsAffected;
		*pRecordset = (*(conn->ms_conn))->Execute(wexecutesql,&RecordsAffected,adCmdText);

		unsigned int num_cols = mssql_field_count(pRecordset);
		create_cursor (L, 1, pRecordset, num_cols);
		lua_pushnumber(L,RecordsAffected.intVal);
		lua_pushstring(L,"");
		lua_pushnil(L);
	}
	catch(_com_error ex)
	{
		// ERROR
		// https://msdn.microsoft.com/en-us/library/ms678037(v=vs.85).aspx
		_bstr_t bstrDescription(ex.Description());
		char errDesc[MAX_ERROR_MSG]={0};
		w2cstatic(errDesc,bstrDescription);
		_bstr_t bstrSource(ex.Source());
		char errSource[MAX_ERROR_MSG] = {0};
		w2cstatic(errSource,bstrSource);
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushfstring(L,"%s %s",errSource,errDesc);
		lua_pushstring(L,"Com Error Exception");
	}
	delete []wexecutesql;
	wexecutesql = NULL;
	return 4;
}


static int conn_query(lua_State *L){
	conn_data *conn = getconnection (L);
	const char *db_querysql = luaL_checkstring(L,2);
	if(db_querysql == NULL ) {
		lua_pushnil(L);
		lua_pushstring(L,"args is error");
		lua_pushnil(L);
		return 3;
	}
	if(adStateOpen != (*(conn->ms_conn))->State){
		lua_pushnil(L);
		lua_pushstring(L,"connect is closed");
		lua_pushnil(L);
		return 3;
	}

	WCHAR *wquerysql = c2w(db_querysql);
	try{
		_CommandPtr    pCommand;
		_RecordsetPtr  *pRecordset = new _RecordsetPtr;
		pCommand.CreateInstance(__uuidof(Command)); 
		pRecordset->CreateInstance(__uuidof(Recordset));
		(*pRecordset)->Open((_bstr_t)wquerysql,_variant_t((IDispatch*)(*(conn->ms_conn))),adOpenStatic,adLockOptimistic,adCmdText);
		if (*pRecordset) {
			unsigned int num_cols = mssql_field_count(pRecordset);
			create_cursor (L, 1, pRecordset, num_cols);
			lua_pushstring(L,"");
			lua_pushnil(L);
		}
		else {
			lua_pushnil(L);
			lua_pushfstring(L,"execute return null %d", GetLastError());
			lua_pushnil(L);
			goto ExitHandler;
		}
	}
	catch(_com_error ex)
	{
		// ERROR
		// https://msdn.microsoft.com/en-us/library/ms678037(v=vs.85).aspx
		_bstr_t bstrDescription(ex.Description());
		char errDesc[MAX_ERROR_MSG]={0};
		w2cstatic(errDesc,bstrDescription);
		_bstr_t bstrSource(ex.Source());
		char errSource[MAX_ERROR_MSG] = {0};
		w2cstatic(errSource,bstrSource);
		lua_pushnil(L);
		lua_pushfstring(L,"%s %s",errSource,errDesc);
		lua_pushstring(L,"Com Error Exception");
		goto ExitHandler;
	}
ExitHandler:
	delete []wquerysql;
	wquerysql = NULL;
	return 3;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/*
** Closes the cursos and nullify all structure fields.
*/
static int cur_nullify (lua_State *L, cur_data *cur) {
	/* Nullify structure fields. */
	cur->closed = 1;
	
	if( adStateOpen == (*(cur->ms_res))->State ) {
		(*cur->ms_res)->Close();
	}

	cur->ms_res->Release();
	delete cur->ms_res;
	cur->ms_res = NULL;

	luaL_unref (L, LUA_REGISTRYINDEX, cur->conn);
	luaL_unref (L, LUA_REGISTRYINDEX, cur->colnames);
	luaL_unref (L, LUA_REGISTRYINDEX, cur->coltypes);
	return 0;
}


/*
** Cursor object collector function
*/
static int cur_gc (lua_State *L) {
	cur_data *cur = (cur_data *)luaL_checkudata (L, 1, LUASQL_CURSOR_MSSQL);
	if (cur != NULL && !(cur->closed))
		cur_nullify (L, cur);
	return 0;
}

/*
** Close the cursor on top of the stack.
** Return 1
*/
static int cur_close (lua_State *L) {
	cur_data *cur = (cur_data *)luaL_checkudata (L, 1, LUASQL_CURSOR_MSSQL);
	luaL_argcheck (L, cur != NULL, 1, LUASQL_PREFIX"cursor expected");
	if (cur->closed) {
		lua_pushboolean (L, 0);
		return 1;
	}
	cur_nullify (L, cur);
	lua_pushboolean (L, 1);
	return 1;
}



/*
** Get the internal database type of the given column.
*/
static char *getcolumntype (VARTYPE type) {
	switch (type)
	{
	case VT_EMPTY:
		return "";
	case VT_NULL:
		return "NULL";
	case VT_UI1:
	case VT_I2:
	case VT_I4:
	case VT_INT:
		return "int";
	case VT_R4:
		return "float";
	case VT_R8:
		return "double";
	case VT_CY:
		return "money";
	case VT_BSTR:
		return "string";
	case VT_DECIMAL:
		return "decimal";
	case VT_BOOL:
		return "boolean";
	case VT_DATE:
		return "date";
	default:
		return "undefined";
	}


	/*switch (type) {
	case MYSQL_TYPE_VAR_STRING: case MYSQL_TYPE_STRING:
		return "string";
	case MYSQL_TYPE_DECIMAL: case MYSQL_TYPE_SHORT: case MYSQL_TYPE_LONG:
	case MYSQL_TYPE_FLOAT: case MYSQL_TYPE_DOUBLE: case MYSQL_TYPE_LONGLONG:
	case MYSQL_TYPE_INT24: case MYSQL_TYPE_YEAR: case MYSQL_TYPE_TINY: 
		return "number";
	case MYSQL_TYPE_TINY_BLOB: case MYSQL_TYPE_MEDIUM_BLOB:
	case MYSQL_TYPE_LONG_BLOB: case MYSQL_TYPE_BLOB:
		return "binary";
	case MYSQL_TYPE_DATE: case MYSQL_TYPE_NEWDATE:
		return "date";
	case MYSQL_TYPE_DATETIME:
		return "datetime";
	case MYSQL_TYPE_TIME:
		return "time";
	case MYSQL_TYPE_TIMESTAMP:
		return "timestamp";
	case MYSQL_TYPE_ENUM: case MYSQL_TYPE_SET:
		return "set";
	case MYSQL_TYPE_NULL:
		return "null";
	default:
		return "undefined";
	}*/
}

/*
** Creates the lists of fields names and fields types.
*/
static void create_colinfo (lua_State *L, cur_data *cur) {
	char pKeyBuffer[50] = {0};
	long idx = 0 ;
	FieldsPtr m_Fields = (*(cur->ms_res))->GetFields();

	lua_newtable (L); /* names */
	lua_newtable (L); /* types */
	for (idx = 1; idx <= m_Fields->Count;idx++)
	{
		FieldPtr m_Field = m_Fields->Item[idx -1 ];
		WCHAR *fieldName = (WCHAR*)m_Field->Name;
		w2cstatic(pKeyBuffer,fieldName);
		VARIANT val = m_Field->GetValue();
		lua_pushstring (L,pKeyBuffer);
		lua_rawseti (L, -3, idx);
		lua_pushfstring(L, "%s",getcolumntype(val.vt));
		lua_rawseti (L, -2, idx);	
	}

	//MYSQL_FIELD *fields;
	//char typename[50];
	//int i;
	//fields = mysql_fetch_fields(cur->my_res);
	//lua_newtable (L); /* names */
	//lua_newtable (L); /* types */
	//for (i = 1; i <= cur->numcols; i++) {
	//	lua_pushstring (L, fields[i-1].name);
	//	lua_rawseti (L, -3, i);
	//	sprintf (typename, "%.20s(%ld)", getcolumntype (fields[i-1].type), fields[i-1].length);
	//	lua_pushstring(L, typename);
	//	lua_rawseti (L, -2, i);
	//}
	/* Stores the references in the cursor structure */
	cur->coltypes = luaL_ref (L, LUA_REGISTRYINDEX);
	cur->colnames = luaL_ref (L, LUA_REGISTRYINDEX);
}


/*
** Pushes a column information table on top of the stack.
** If the table isn't built yet, call the creator function and stores
** a reference to it on the cursor structure.
*/
static void _pushtable (lua_State *L, cur_data *cur, size_t off) {
	int *ref = (int *)((char *)cur + off);

	/* If colnames or coltypes do not exist, create both. */
	if (*ref == LUA_NOREF)
		create_colinfo(L, cur);

	/* Pushes the right table (colnames or coltypes) */
	lua_rawgeti (L, LUA_REGISTRYINDEX, *ref);
}
#define pushtable(L,c,m) (_pushtable(L,c,offsetof(cur_data,m)))

/*
** Check for valid cursor.
*/
static cur_data *getcursor (lua_State *L) {
	cur_data *cur = (cur_data *)luaL_checkudata (L, 1, LUASQL_CURSOR_MSSQL);
	luaL_argcheck (L, cur != NULL, 1, "cursor expected");
	luaL_argcheck (L, !cur->closed, 1, "cursor is closed");
	return cur;
}

/*
** Return the list of field names.
*/

static int cur_getcolnames (lua_State *L) {
	cur_data *cur = getcursor (L);
	_RecordsetPtr res = (*(cur->ms_res));
	if( res->State == adStateOpen ) {
		pushtable (L, getcursor(L), colnames);
		lua_pushstring(L,"");
	} else {
		lua_pushnil(L);
		lua_pushstring(L,"RecordsetPtr state is not open");
	}
	return 2;
}

/*
** Return the list of field types.
*/
static int cur_getcoltypes (lua_State *L) {
	cur_data *cur = getcursor (L);
	_RecordsetPtr res = (*(cur->ms_res));
	if( res->State == adStateOpen ) {
		pushtable (L, getcursor(L), coltypes);
		lua_pushstring(L,"");
	} else {
		lua_pushnil(L);
		lua_pushstring(L,"RecordsetPtr state is not open");
	}
	return 2;
}


/*
** Push the number of rows.
*/
static int cur_numrows (lua_State *L) {
	cur_data *cur = getcursor (L);
	_RecordsetPtr res = (*(cur->ms_res));
	if( res->State == adStateOpen ) {
		lua_pushinteger (L, res->RecordCount);
		lua_pushstring(L,"");
	} else {
		lua_pushnil(L);
		lua_pushstring(L,"RecordsetPtr state is not open");
	}
	return 2;
}




/*
** Get another row of the given cursor.
*/
static int cur_fetch (lua_State *L) {
	cur_data *cur = getcursor (L);
	_RecordsetPtr res = (*(cur->ms_res));

	if( res->State != adStateOpen )
	{
		lua_pushnil(L);
		lua_pushstring(L,"RecordsetPtr state is not open");
		return 2;
	}

	//unsigned long *lengths;
	//MYSQL_ROW row = mysql_fetch_row(res);
	//if (row == NULL) {
	//	cur_nullify (L, cur);
	//	lua_pushnil(L);  /* no more results */
	//	return 1;
	//}
	//lengths = mysql_fetch_lengths(res);

	if (lua_istable (L, 2)) {
		//res:fetch({}, 'a')
		const char *opts = luaL_optstring (L, 3, "n");
		//if (strchr (opts, 'n') != NULL) {
		//	/* Copy values to numerical indices */
		//	int i;
		//	for (i = 0; i < cur->numcols; i++) {
		//		pushvalue (L, row[i], lengths[i]);
		//		lua_rawseti (L, 2, i+1);
		//	}
		//}
		if (strchr (opts, 'a') != NULL) {
#if 0
			//debug 
			//int i;
			// Check if colnames exists 
			if (cur->colnames == LUA_NOREF){
				pushtable (L, getcursor(L), colnames);
				lua_rawgeti (L, LUA_REGISTRYINDEX, cur->colnames);
			}
			lua_pushnil(L);
			return 2;
#endif

			lua_newtable(L);
			int nArray = lua_gettop(L);
			int nCols = 0 ;

			//////////////////////////////////////////////////////////////////////////
			while(!res->adoEOF)
			{
				char pKeyBuffer[KEY_MAX_LENGTH] = {0};
				char pValBuffer[VAL_MAX_LENGTH] = {0};
				lua_newtable(L);
				int nField = lua_gettop(L);

				FieldsPtr m_Fields = res->GetFields();

				std::string tempstr;

				for (long idx = 0;idx < m_Fields->Count;idx++)
				{
					FieldPtr m_Field = m_Fields->Item[idx];
					WCHAR *fieldName = (WCHAR*)m_Field->Name;
					w2cstatic(pKeyBuffer,fieldName);
					lua_pushlstring(L, pKeyBuffer, (strlen(pKeyBuffer)/sizeof(char)));

					VARIANT val = m_Field->GetValue();

					switch(val.vt)
					{
					case VT_EMPTY:
						lua_pushstring(L,"");
						break;

					case VT_NULL:
						lua_pushstring(L,"NULL");
						break;

					case VT_UI1:
					case VT_I2:
					case VT_I4:
					case VT_INT:
						lua_pushnumber(L,val.intVal);
						break;
					case VT_R4:
						lua_pushnumber(L,val.fltVal);
						break;
					case VT_R8:
						lua_pushnumber(L,val.dblVal);
						break;
					case VT_CY:
						::VariantChangeType(&val,&val,0,VT_BSTR);
						w2cstatic(pValBuffer,val.bstrVal );
						lua_pushstring(L,pValBuffer);
						break;
					case VT_BSTR:
						w2cstatic(pValBuffer,val.bstrVal);
						lua_pushstring(L,pValBuffer);
						break;
					case VT_DECIMAL:
						::VariantChangeType(&val,&val,0,VT_BSTR);
						w2cstatic(pValBuffer,val.bstrVal);
						lua_pushstring(L,pValBuffer);
						break;
					case VT_BOOL:
						lua_pushboolean(L,val.bVal);
						break;
					case VT_DATE:
						::VariantChangeType(&val,&val,0,VT_BSTR);
						w2cstatic(pValBuffer,val.bstrVal);
						tempstr = pValBuffer;
						// must replace / to -
						tempstr = tempstr.replace(tempstr.find("/"), 1, "-");
						tempstr = tempstr.replace(tempstr.find("/"), 1, "-");
						lua_pushstring(L,tempstr.c_str());
						break;
					default:
						lua_pushnil(L);
						printf("*****key:%s type:%08X read Failed! \r\n",pKeyBuffer , val.vt );
						break;
					}
					lua_rawset(L,nField);
				}

				lua_pushvalue(L,nField);
				lua_rawseti(L,nArray,++nCols);
				lua_settop(L,nArray);
				res->MoveNext();
			}
			lua_pushnil(L);
			//////////////////////////////////////////////////////////////////////////
		}
	}
	else {
		/*int i;
		luaL_checkstack (L, cur->numcols, LUASQL_PREFIX"too many columns");
		for (i = 0; i < cur->numcols; i++)
		pushvalue (L, row[i], lengths[i]);*/

		// return number for cols
		lua_pushnumber(L,cur->numcols);
		lua_pushnil(L);
	}
	return 2;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/*
** Create metatables for each class of object.
*/
static void create_metatables (lua_State *L) {
	struct luaL_Reg environment_methods[] = {
		{"__gc", env_gc},
		{"close", env_close},
		{"connect", env_connect},
		{NULL, NULL},
	};
	struct luaL_Reg connection_methods[] = {
		{"__gc", conn_gc},
		{"close", conn_close},
		//{"escape", escape_string},
		{"query", conn_query},		//查询
		{"execute", conn_execute},
		{"commit", conn_commit},
		{"rollback", conn_rollback},
		{"begintrans", conn_begintrans},
		//{"getlastautoid", conn_getlastautoid},*/
		{NULL, NULL},
	};
	struct luaL_Reg cursor_methods[] = {
		{"__gc", cur_gc},
		{"close", cur_close},
		{"getcolnames", cur_getcolnames},
		{"getcoltypes", cur_getcoltypes},
		{"fetch", cur_fetch},
		{"numrows", cur_numrows},
		{NULL, NULL},
	};
	luasql_createmeta (L, LUASQL_ENVIRONMENT_MSSQL, environment_methods);
	luasql_createmeta (L, LUASQL_CONNECTION_MSSQL, connection_methods);
	luasql_createmeta (L, LUASQL_CURSOR_MSSQL, cursor_methods);
	lua_pop (L, 3);
}


__declspec(dllexport) int luaopen_luasqlserver(lua_State* L)  
{
	struct luaL_Reg driver[] = {
		{"mssql", create_environment},
		{NULL, NULL},
	};
	create_metatables (L);
	lua_newtable(L);
	luaL_setfuncs(L, driver, 0);

	luasql_set_info (L);
	lua_pushliteral (L, "_MSSQLVERSION");
	lua_pushliteral (L, MSSQL_SERVER_VERSION);
	lua_settable (L, -3);

	return 1;
}