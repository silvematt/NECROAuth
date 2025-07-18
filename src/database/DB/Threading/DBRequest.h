#ifndef NECRO_DB_REQUEST_RESULT_H
#define NECRO_DB_REQUEST_RESULT_H

#include <cstdint>
#include <memory>
#include <functional> 

#include <mysqlx/xdevapi.h>

class DBRequest
{
public:
	bool									m_done = false;
	bool									m_fireAndForget;
	mysqlx::SqlStatement					m_sqlStmt;
	std::vector<uint8_t>					m_pcktData;
	mysqlx::SqlResult						m_sqlRes;
	std::function<bool(mysqlx::SqlResult&)>	m_callback;

	std::function<void()>					m_noticeFunc;


	DBRequest(bool fireAndForget, mysqlx::SqlStatement stmt) : m_fireAndForget(fireAndForget), m_sqlStmt(std::move(stmt))
	{
		m_done = false;
		m_callback = nullptr;
	}
};

#endif
