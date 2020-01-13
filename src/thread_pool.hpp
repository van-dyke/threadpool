
#ifndef THREAD_POOL_HPP_
#define THREAD_POOL_HPP_

#include <vector>
#include <thread>
#include <queue>
#include <memory>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <future>
#include <functional>


namespace ThreadPool
{

	class thread_pool
	{

	public:

		class join_threads
		{
			friend class thread_pool;

		public:

			join_threads(join_threads const&) = delete;
			join_threads& operator=(join_threads const&) = delete;

			~join_threads()
			{
				for(unsigned long i=0; i<threads.size(); ++i)
				{
					std::cout << "Joining thread " << i << "\n";
					if(threads[i].joinable())
						threads[i].join();
				}
			}

		private:

			explicit join_threads(std::vector<std::thread>& threads_): threads(threads_)
			{}


			std::vector<std::thread>& threads;


		};


		thread_pool() : done(false), joiner(std::unique_ptr<join_threads>(new join_threads(worker_threads)))
		{
			unsigned const thread_count = std::thread::hardware_concurrency();

			std::cout << "Pool size: " << thread_count << " threads..." << std::endl;

			try
			{
				for(unsigned i = 0; i < thread_count; ++i)
				{
					worker_threads.emplace_back([this]{ thread_func1(); });
					//worker_threads.push_back( std::thread(&thread_pool::thread_func, this));
				}
			}
			catch(...)
			{
				done = true;
				throw;
			}
		}

		~thread_pool()
		{
			std::cout << "Clean up... " << std::endl;
			done = true;
		    condition.notify_all();
		}

		thread_pool(thread_pool const &) = delete;
		thread_pool(thread_pool&&) = delete;
		thread_pool& operator=(thread_pool const&) = delete;


		template<typename Func, typename... Args>
		auto submit(Func&& f, Args&&... args) // -> std::future<typename std::result_of<F(Args...)>::type>
		{
			using result_type = std::result_of_t<Func(Args...)>;

			auto task = std::make_shared< std::packaged_task<result_type()> >(
			            std::bind(std::forward<Func>(f), std::forward<Args>(args)...) );

			std::future<result_type> res = task->get_future();

			{
				std::unique_lock<std::mutex> lock(this->queue_mutex);

				if(done)
					throw std::runtime_error("enqueue on stopped ThreadPool");

				tasks.emplace([task](){ (*task)(); });
			}

			condition.notify_one();
			return res;
		}

	private:

		void thread_func1()
		{
			for(;;)
			{
				std::function<void()> task;
				{
					std::unique_lock<std::mutex> lock(this->queue_mutex);
					this->condition.wait(lock,[this]{ return this->done || !this->tasks.empty(); });

					if(this->done && this->tasks.empty())
						return;

					task = std::move(this->tasks.front());
					this->tasks.pop();
                }

				task();

			}
		}

		void thread_func2()
		{
			while(!done)
			{
				std::function<void()> task;

				std::unique_lock<std::mutex> lock(this->queue_mutex);
				if( this->tasks.empty() )
				{
					std::this_thread::yield();
				}
				else
				{
					task = std::move(this->tasks.front());
					this->tasks.pop();
					task();
				}
			}
		}

		std::atomic<bool> done;
		std::mutex queue_mutex;
		std::condition_variable condition;

		std::vector<std::thread> worker_threads;
		std::queue< std::function<void()> > tasks;
		std::unique_ptr<join_threads> joiner;


	};
}



#endif /* THREAD_POOL_HPP_ */
