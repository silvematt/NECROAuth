#ifndef NECRO_DATABASE_WORKER
#define NECRO_DATABASE_WORKER

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>

#include "Database.h"

//-----------------------------------------------------------------------------------------------------
// An abstraction of a thread that works on a database
//-----------------------------------------------------------------------------------------------------
class DatabaseWorker
{
private:
	std::unique_ptr<Database> db;
	std::thread thread;

	std::mutex queueMutex;
	int qSize = 0;
	std::queue<mysqlx::SqlStatement> q;
	std::condition_variable cond;

	std::atomic<bool> running{ false };

public:
	void Setup(Database::DBType t)
	{
		switch (t)
		{
			case Database::DBType::LOGIN_DATABASE:
				db = std::make_unique<LoginDatabase>();
				break;

			default:
				throw std::exception("No database type!");
		}

		db->Init();
	}

	void Start()
	{
		if (thread.joinable())
		{
			std::cout << "Attempting to start while thread is still joinable. This should not happen. Trying to stop and join..." << std::endl;
			Stop();
			Join();
		}

		running = true;
		thread = std::thread(&DatabaseWorker::ThreadRoutine, this);
	}

	void Stop()
	{
		std::lock_guard<std::mutex> lock(queueMutex);

		running = false;
		cond.notify_all();
	}

	void Join()
	{
		if(thread.joinable())
			thread.join();
	}

	void Enqueue(mysqlx::SqlStatement&& s)
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		q.push(std::move(s));
		qSize++;

		cond.notify_all();
	}

	void ThreadRoutine()
	{
		while (true)
		{
			std::unique_lock<std::mutex> lock(queueMutex);

			// If we Stopped the thread and there's nothing in the queue, exit
			if (!running && qSize <= 0)
			{
				lock.unlock();
				break;
			}

			// Otherwise, we're either still running or there's still something in the queue
			if (qSize <= 0) // if we're still running and queue is empty
			{
				// Sleep
				while (qSize <= 0 && running)
					cond.wait(lock);
			}
			else // we have something to run
			{
				// We have lock here

				mysqlx::SqlStatement stmt = std::move(q.front());
				q.pop();
				qSize--;

				lock.unlock();

				// Do stuff
				try
				{
					std::cout << "executing one" << std::endl;
					stmt.execute();
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
		}
	}
};

#endif
