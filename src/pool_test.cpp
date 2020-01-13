//============================================================================
// Name        : thread_pool.cpp
// Author      : Lukasz Warzywoda
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <numeric>
#include "thread_pool.hpp"
using namespace std;
using namespace ThreadPool;

template<typename Iterator, typename T>
struct accumulate_block
{
	T operator()(Iterator begin, Iterator end)
	{
		return std::accumulate(begin, end, T());
	}
};

template<typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init)
{
	unsigned long const length = std::distance(first, last);
	if(!length)
		return init;

	unsigned long const block_size = 25;
	unsigned long const num_blocks = (length+block_size-1)/block_size;

	std::vector<std::future<T> > futures(num_blocks-1);
	thread_pool pool;
	Iterator block_start=first;

	for(unsigned long i=0; i<(num_blocks-1); ++i)
	{
		Iterator block_end = block_start;
		std::advance(block_end, block_size);
		futures[i] = pool.submit( [&](){ return accumulate_block<Iterator,T>()(block_start, block_end); } );
		block_start = block_end;
	}
	T last_result = accumulate_block<Iterator,T>()(block_start, last);
	T result = init;

	for(unsigned long i = 0;i < (num_blocks-1); ++i)
	{
		result += futures[i].get();
	}

	result += last_result;
	return result;

}

int main() {

	thread_pool t{};


	std::vector<std::future<unsigned>> futures(8);


	for(auto &&f : futures)
		f = t.submit( [](unsigned answer) { for(unsigned i = 0; i < 1000000000; ++i)
												answer += 1;
											return answer+100; }, 42 );

	for(auto &&f : futures)
		std::cout << "Result: " << f.get() << "\n";

	std::vector<int> v(100);
    std::generate(v.begin(), v.end(), [](){ return rand()%10; });

    std::cout << "Accumulate test\n";

	auto res = parallel_accumulate(v.begin(), v.end(), 0);

	std::cout << res << std::endl;

	return 0;
}
