
#include "Matrix.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>


using namespace std::literals::chrono_literals;


Matrix * stage_one_data = nullptr;
std::mutex stage_one_data_protector;
std::condition_variable stage_one_data_change;
std::atomic<bool> should_end;

void stage_one_thread() {

	for (size_t i = 0; i < MATRIX_COUNT; i++)
	{
		{
			std::lock_guard<std::mutex> guard(stage_one_data_protector);
			stage_one_data = new Matrix(i);
			std::cout << "[" << i << "/" << MATRIX_COUNT <<"]" << " Stage one thread has just generated matrix with id: " << stage_one_data->id << std::endl;
		}
		stage_one_data_change.notify_all();

		//and now wait for others
		{
			std::unique_lock<std::mutex> unique(stage_one_data_protector);
			stage_one_data_change.wait(unique, [] {return stage_one_data ==nullptr; });
		}
	}

	should_end = true;
}


void acceptor_thread() {


	while (!should_end) {
		std::unique_lock<std::mutex> lk(stage_one_data_protector);
		stage_one_data_change.wait(lk, [] {return stage_one_data != nullptr; });
	
		std::cout << std::this_thread::get_id() << " processing" << std::endl;
		std::this_thread::sleep_for(1s);
		stage_one_data = nullptr;

		lk.unlock();
		stage_one_data_change.notify_all();
		
		//and now wait for others
		{
			std::unique_lock<std::mutex> unique(stage_one_data_protector);
			stage_one_data_change.wait(unique, [] {return stage_one_data == nullptr; });
		}

	}


}

int main()
{
	std::thread a(acceptor_thread);
	std::thread aa(acceptor_thread);
	std::thread aaa(acceptor_thread);
	std::thread b(stage_one_thread);

	a.join();
	aa.join();
	aaa.join();
	b.join();


}

