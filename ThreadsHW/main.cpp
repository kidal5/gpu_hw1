
#include "Matrix.h"

#include <assert.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>


using namespace std::literals::chrono_literals;

std::mutex cout_mutex;

Matrix * stage_one_data = nullptr;
std::mutex stage_one_data_protector;
std::condition_variable stage_one_data_change;
bool stage_one_should_end;

void stage_one_thread() {

	for (size_t i = 0; i < MATRIX_COUNT; i++)
	{
		{
			std::lock_guard<std::mutex> guard(stage_one_data_protector);
			stage_one_data = new Matrix(i);
			{
				std::lock_guard<std::mutex> cout(cout_mutex);
				std::cout << "[" << i << "/" << MATRIX_COUNT << "]" << " Stage one: Generated matrix with id: " << stage_one_data->id << std::endl;
				std::cout << std::endl;
			}
		}
		stage_one_data_change.notify_all();

		//and now wait for others
		{
			std::unique_lock<std::mutex> unique(stage_one_data_protector);
			stage_one_data_change.wait(unique, [] {return stage_one_data == nullptr; });
		}
	}

	stage_one_should_end = true;
	stage_one_data_change.notify_all();
	{
		std::lock_guard<std::mutex> cout(cout_mutex);
		std::cout << "Stage one: Thread ended" << std::endl;
		std::cout << std::endl;
	}
}

int stage_two_inner_buffer_count = 0;
Matrix * stage_two_inner_buffer[STAGE2_BUFFER_SIZE];
std::mutex stage_two_inner_buffer_protector;
std::condition_variable stage_two_inner_buffer_empty; //aka is some free space
std::condition_variable stage_two_inner_buffer_full; //aka is some job to be done
bool stage_two_should_end = false;

int stage_two_public_buffer_count = 0;
Matrix * stage_two_public_buffer[STAGE2_PUBLIC_BUFFER_SIZE];
std::mutex stage_two_public_buffer_protector;
std::condition_variable stage_two_public_buffer_empty; //aka is some free space
std::condition_variable stage_two_public_buffer_full; //aka is some job to be done

void stage_two_worker_thread() {

	Matrix * current_data;

	while (true) {
		//wait for task
		//wait for buffer_empty in
		std::unique_lock<std::mutex> lk_full(stage_two_inner_buffer_protector);
		stage_two_inner_buffer_full.wait(lk_full, [] {return stage_two_inner_buffer_count > 0 || stage_two_should_end; });

		if (stage_two_inner_buffer_count == 0) break;

		//take data into local memory
		current_data = stage_two_inner_buffer[stage_two_inner_buffer_count - 1];
		stage_two_inner_buffer[stage_two_inner_buffer_count - 1] = nullptr;
		stage_two_inner_buffer_count--;

		{
			std::lock_guard<std::mutex> cout(cout_mutex);
			std::cout << "Stage two inner: Matrix with id: " << current_data->id << " has been placed OUT of inner buffer." << std::endl;
			std::cout << "Stage two inner: Current inner buffer status [" << stage_two_inner_buffer_count << "/" << STAGE2_BUFFER_SIZE << "]" << std::endl;
			std::cout << std::endl;
		}

		//notify boss
		lk_full.unlock();
		stage_two_inner_buffer_empty.notify_one();

		//do its job
		current_data->make_triangle_form();

		//wait for space in public buffer
		std::unique_lock<std::mutex> lk_empty(stage_two_public_buffer_protector);
		stage_two_public_buffer_empty.wait(lk_empty, [] {return stage_two_public_buffer_count < STAGE2_PUBLIC_BUFFER_SIZE; });

		//take data and put into buffer
		assert(stage_two_public_buffer[stage_two_public_buffer_count] == nullptr);
		stage_two_public_buffer[stage_two_public_buffer_count] = current_data;
		stage_two_public_buffer_count++;
		current_data = nullptr;

		{
			std::lock_guard<std::mutex> cout(cout_mutex);
			std::cout << "Stage two public: Matrix with id: " << stage_two_public_buffer[stage_two_public_buffer_count - 1]->id << " has been placed INTO public buffer." << std::endl;
			std::cout << "Stage two public: Current public buffer status [" << stage_two_public_buffer_count << "/" << STAGE2_PUBLIC_BUFFER_SIZE << "]" << std::endl;
			std::cout << std::endl;
		}

		lk_empty.unlock();
		stage_two_public_buffer_empty.notify_one();
	}

	{
		std::lock_guard<std::mutex> cout(cout_mutex);
		std::cout << "Stage two: Worker thread has ended." << std::endl;
		std::cout << std::endl;
	}

}

void stage_two_boss_thread() {

	//first create its own threads...
	std::thread threads[STAGE2_WORKERS_COUNT];
	for (size_t i = 0; i < STAGE2_WORKERS_COUNT; i++)
		threads[i] = std::thread(stage_two_worker_thread);

	bool should_work = true;

	while (true) {
		//this guy has to wait for two conditions
		//wait for data ready in stage one
		std::unique_lock<std::mutex> lk_one(stage_one_data_protector);
		stage_one_data_change.wait(lk_one, [] {return stage_one_data != nullptr || stage_one_should_end;  });

		if (stage_one_data == nullptr) break;
	
		//wait for buffer_empty in
		std::unique_lock<std::mutex> lk_empty(stage_two_inner_buffer_protector);
		stage_two_inner_buffer_empty.wait(lk_empty, [] {return stage_two_inner_buffer_count < STAGE2_BUFFER_SIZE; });

		//take data and put into buffer
		assert(stage_two_inner_buffer[stage_two_inner_buffer_count] == nullptr);
		stage_two_inner_buffer[stage_two_inner_buffer_count] = stage_one_data;
		stage_two_inner_buffer_count++;
		stage_one_data = nullptr;

		{
			std::lock_guard<std::mutex> cout(cout_mutex);
			std::cout << "Stage two inner: Matrix with id: " << stage_two_inner_buffer[stage_two_inner_buffer_count - 1]->id << " has been placed INTO inner buffer." << std::endl;
			std::cout << "Stage two inner: Current inner buffer status [" << stage_two_inner_buffer_count << "/" << STAGE2_BUFFER_SIZE << "]" << std::endl;
			std::cout << std::endl;
		}
		//notify workers
		lk_empty.unlock();
		stage_two_inner_buffer_full.notify_all();

		//notify stage one worker 
		lk_one.unlock();
		stage_one_data_change.notify_all();

	}


	//propagat end condition
	stage_two_should_end = true;
	stage_two_inner_buffer_full.notify_all();

	for (size_t i = 0; i < STAGE2_WORKERS_COUNT; i++)
		threads[i].join();

	{
		std::lock_guard<std::mutex> cout(cout_mutex);
		std::cout << "Stage two: Boss thread has ended." << std::endl;
		std::cout << std::endl;
	}

}



void stage_three_thread() {



}


int main()
{
	std::thread a(stage_two_boss_thread);
	std::thread b(stage_one_thread);


	a.join();
	b.join();


}

