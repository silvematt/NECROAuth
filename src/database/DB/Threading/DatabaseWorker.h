#ifndef NECRO_DATABASE_WORKER_H
#define NECRO_DATABASE_WORKER_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>

#include "LoginDatabase.h"
#include "DBRequest.h"
#include <windows.h>
namespace NECRO
{
	//-----------------------------------------------------------------------------------------------------
	// An abstraction of a thread that works on a database
	//-----------------------------------------------------------------------------------------------------
	class DatabaseWorker
	{
	private:
		std::unique_ptr<Database>	m_db;
		std::thread					m_thread;

		std::atomic<bool> m_running{ false };

		// !! Shared Members (access with mutex) !!
		// Queue used to enqueue requests given by the main thread to be executed by this worker thread
		std::mutex				m_executionMutex;
		std::condition_variable	m_execWakeupCond;
		std::queue<DBRequest>	m_execQueue;
		int						m_execQueueSize = 0;

		// !! Shared Members (access with mutex) !!
		// DBWorker saves Requests that require a callback to be executed upon a SQL Response in the m_respQueue.
		// DB Worker will insert the request in teh queue and the pooling mechanism of the server will allow the DBThread to wake up the main thread and 
		// make it check this queue. The callback will be executed on the associated AuthSession object that made the request in the first place.
		std::mutex				m_respMutex;
		std::queue<DBRequest>	m_respQueue;

	public:
		int Setup(Database::DBType t)
		{
			switch (t)
			{
			case Database::DBType::LOGIN_DATABASE:
				m_db = std::make_unique<LoginDatabase>();
				break;

			default:
				throw std::exception("No database type!");
			}

			if (m_db->Init() == 0)
				return 0;
			else
			{
				LOG_ERROR("Could not initialize DatabaseWorker internal db, MySQL may be not running.");
				return 1;
			}
		}

		//-----------------------------------------------------------------------------------------------------
		// Starts the thread with its ThreadRoutine
		//-----------------------------------------------------------------------------------------------------
		int Start()
		{
			if (m_thread.joinable())
			{
				LOG_WARNING("Attempting to start while thread is still joinable. This should not happen. Trying to stop and join.");
				Stop();
				Join();
			}

			m_running = true;
			m_thread = std::thread(&DatabaseWorker::ThreadRoutine, this);

			return 0;
		}

		//-----------------------------------------------------------------------------------------------------
		// Stops the ThreadRoutine when the queue will empty out
		//-----------------------------------------------------------------------------------------------------
		void Stop()
		{
			{
				std::lock_guard<std::mutex> lock(m_executionMutex);

				m_running = false;
			}
			m_execWakeupCond.notify_all();
		}

		// ----------------------------------------------------------------------------------------------------
		// Joins the thread when it's possible
		//-----------------------------------------------------------------------------------------------------
		void Join()
		{
			if (m_thread.joinable())
				m_thread.join();
		}

		// ----------------------------------------------------------------------------------------------------
		// Closes the DBConnection session (should happen after worker Joins)
		//-----------------------------------------------------------------------------------------------------
		void CloseDB()
		{
			m_db->Close();
		}

		void Enqueue(DBRequest&& r)
		{
			std::lock_guard<std::mutex> lock(m_executionMutex);
			m_execQueue.push(std::move(r));
			m_execQueueSize++;

			m_execWakeupCond.notify_all();
		}

		mysqlx::SqlStatement Prepare(int enum_val)
		{
			return m_db->Prepare(enum_val);
		}

		std::queue<DBRequest> GetResponseQueue()
		{
			std::lock_guard guard(m_respMutex);

			std::queue<DBRequest> toReturn;
			std::swap(toReturn, m_respQueue);
			return toReturn;
		}

		void ThreadRoutine()
		{
			while (true)
			{
				std::unique_lock<std::mutex> lock(m_executionMutex);

				// If we Stopped the thread and there's nothing in the queue, exit
				if (!m_running && m_execQueueSize <= 0)
				{
					lock.unlock();
					break;
				}

				// Otherwise, we're either still running or there's still something in the queue
				if (m_execQueueSize <= 0) // if we're still running and queue is empty
				{
					// Sleep
					while (m_execQueueSize <= 0 && m_running)
						m_execWakeupCond.wait(lock);
				}
				else // we have something to run
				{
					// We have lock here

					DBRequest req = std::move(m_execQueue.front());
					m_execQueue.pop();
					m_execQueueSize--;

					lock.unlock();

					// Do stuff
					try
					{
						req.m_sqlRes = req.m_sqlStmt.execute();
						// Simulate load
						Sleep(3000);

						// Check if this request needs to trigger a callback, if so, we enqueue it in the response queue
						if (!req.m_fireAndForget)
						{
							std::unique_lock<std::mutex> resGuard(m_respMutex);

							
							// Preserve the life of the m_noticeFunc in this scope before it gets moved
							std::function<void()> func = std::move(req.m_noticeFunc);
							m_respQueue.push(std::move(req));

							resGuard.unlock();

							// Call notice function if set
							if (func)
								func();
						}
					}
					catch (const mysqlx::Error& err)  // catches MySQL Connector/C++ specific exceptions
					{
						std::cerr << "DBWorker MySQL error: " << err.what() << std::endl;
					}
					catch (const std::exception& ex)  // catches standard exceptions
					{
						std::cerr << "DBWorker Standard exception: " << ex.what() << std::endl;
					}
					catch (...)
					{
						std::cerr << "DBWorker Unknown exception caught!" << std::endl;
					}
				}
			}
		}
	};

}

#endif
