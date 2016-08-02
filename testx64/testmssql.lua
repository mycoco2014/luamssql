local libsql = require('luasqlserver')
local cjson = require('cjson')


local function get_conn()
	local env = libsql.mssql()
	local strfmt = "Provider=SQLOLEDB.1;Data Source=%s;database=%s;uid=%s;pwd=%s"
	local strval = string.format(strfmt,"127.0.0.1","testDemo","sa","sa")

	local conn, ret = env:connect(strval,5)
	env:close()
	return conn, ret
end

local function test_mssql_select(conn)
	local res, rows, err,exp = conn:query("select top 2 * from sysobjects  ")
	print(res, rows, err,exp)
	if type(res) == "userdata" then
		-- local ok, err = res:getcolnames()
		-- print(ok,err)
		-- print(cjson.encode(ok))
		-- local ok, err = res:getcoltypes()
		-- print(cjson.encode(ok))
		local rows = res:numrows()
		print("rows",rows)
		local ret, err, ex  = res:fetch({}, 'a') or {}
		print(ret, err, ex)
		if type(ret) == "table" then
			print("=====",#ret)
		end
		print(cjson.encode(ret))
		res:close()
	end
end



local function test_mssql_insert(conn)
	local cur_time = os.time()
	local fmt = "insert into luaTestUser(card,crc32,idx,createTimestamp,updateTimestamp,createTime,updateTime,amount,remark)" ..
					"values('%s',0,1,%d,%d,getdate(),getdate(),0,'xxx')"
	local sql = string.format(fmt,cur_time,cur_time,cur_time)
	conn:begintrans()
	local rs,row,err,exp = conn:execute(sql)
	---conn:rollback()
	conn:commit()
	
	if type(rs) == "userdata" then
		local res,err = rs:fetch({},'a')
		print(res)
		if type(res) == "table" then
			print(cjson.encode(res))
		end
		rs:close()
	end

	print(rs,row,err,exp)
	return ok, err
end

local function test_mssql_update(conn)
	local sql = string.format("update luaTestUser set updateTimestamp=%d where card ='1470133708' ", os.time())

	print(sql)

	conn:begintrans()
	local rs,row,err,exp = conn:execute(sql)
	-- conn:rollback()
	conn:commit()

	print(rs,row,err,exp)
	return ok, err
end


local function test_mssql_delete(conn)
	local sql = string.format("delete from luaTestUser  where card ='1470133924' ", os.time())

	print(sql)

	conn:begintrans()
	local rs,row,err,exp = conn:execute(sql)
	-- conn:rollback()
	conn:commit()

	print(rs,row,err,exp)
	return ok, err
end

local function test_mssql_proc(conn)
	local sql = string.format(" exec sp_test_proc 10 ", os.time())

	print(sql)

	-- conn:begintrans()
	local rs,row,err,exp = conn:execute(sql)
	-- conn:rollback()
	-- conn:commit()

	if type(rs) == "userdata" then
		local ret,err = rs:fetch({},'a')
		if type(ret) == "table" then
			print(cjson.encode(ret))
		end
	end

	print(rs,row,err,exp)
	return ok, err
end


local function test_main()
	local conn, err = get_conn()
	if not conn then
		print('error')
		return 
	end

	-- test_mssql_select(conn)
	-- test_mssql_insert(conn)
	-- test_mssql_update(conn)
	-- test_mssql_delete(conn)
	test_mssql_proc(conn)

	conn:close()
	print("ok")
end

test_main()
