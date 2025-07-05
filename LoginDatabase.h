#ifndef NECRO_LOGIN_DATABASE
#define NECRO_LOGIN_DATABASE

#include "DBConnection.h"

//-------------------------------------------------------
// Enum of all possible statements
//-------------------------------------------------------
enum LoginDatabaseStatements : uint32_t
{
	LOGIN_SEL_ACCOUNT_ID_BY_NAME, // name(string)
	LOGIN_UPD_ON_LOGIN
};


//-----------------------------------------------------------------------------------------------------
// Wrapper for login database connection
//-----------------------------------------------------------------------------------------------------
class LoginDatabase
{
private:
	DBConnection conn;


public:
	int Init()
	{
		if (conn.Init("localhost", 33060, "root", "root") == 0)
			return 0;
		else 
			return -1;
	}


	//-----------------------------------------------------------------------------------------------------
	// Returns a mysqlx::SqlStatement, ready to be bound with parameters and executed by the caller
	//-----------------------------------------------------------------------------------------------------
	mysqlx::SqlStatement Prepare(LoginDatabaseStatements s)
	{
		switch (s)
		{
			case LOGIN_SEL_ACCOUNT_ID_BY_NAME:
				return conn.session->sql("SELECT id FROM necroauth.users WHERE username = ?;");

			case LOGIN_UPD_ON_LOGIN:
				// TODO
				break;

			default:
				throw std::invalid_argument("Invalid LoginDatabaseStatement");
				break;
		}
	}


	//-----------------------------------------------------------------------------------------------------
	// Executes a SqlStatement in a try-catch block and returns a SqlResult
	//-----------------------------------------------------------------------------------------------------
	mysqlx::SqlResult Execute(mysqlx::SqlStatement& statement)
	{
		try
		{
			return statement.execute();
		}
		catch (const mysqlx::Error& err)  // catches MySQL Connector/C++ specific exceptions
		{
			std::cerr << "MySQL error: " << err.what() << std::endl;
		}
		catch (const std::exception& ex)  // catches standard exceptions
		{
			std::cerr << "Standard exception: " << ex.what() << std::endl;
		}
		catch (...)
		{
			std::cerr << "Unknown exception caught!" << std::endl;
		}
	}
};

#endif
