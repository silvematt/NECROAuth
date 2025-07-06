#ifndef NECRO_DATABASE
#define NECRO_DATABASE

#include "DBConnection.h"


//-----------------------------------------------------------------------------------------------------
// Database basic definition
//-----------------------------------------------------------------------------------------------------
class Database
{
public:
	enum DBType
	{
		LOGIN_DATABASE = 0
	};


protected:
	DBConnection conn;


public:
	virtual int Init() = 0;
	virtual mysqlx::SqlStatement Prepare(int enum_val) = 0;
	virtual mysqlx::SqlResult Execute(mysqlx::SqlStatement& statement) = 0;
};

#endif
