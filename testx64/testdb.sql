
if not exists(select top 1 1 from sysobjects where name ='luaTestUser' and xtype='u')
begin
	CREATE TABLE [dbo].[luaTestUser](
		[id] [int] IDENTITY(1,1) NOT NULL,
		[card] [varchar](18) NOT NULL,
		[crc32] [int] NOT NULL,
		[idx] [int] NOT NULL,
		[createTimestamp] [int] NOT NULL,
		[updateTimestamp] [int] NOT NULL,
		[createTime] [datetime] NOT NULL,
		[updateTime] [datetime] NOT NULL,
		[amount] [money] NOT NULL,
		[remark] [nvarchar](128) NOT NULL
	)
end
go

if exists(select top 1 1 from sysobjects where name ='sp_test_proc' and xtype='p')
	drop proc sp_test_proc
go
create proc sp_test_proc(@maxid int)
as
	SET NOCOUNT ON
	begin
		select top 10 * from luaTestUser where id <= @maxid
	end
go

