
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
}

int stage_two_buffer_count = 0;
Matrix * stage_two_buffer[STAGE2_BUFFER_SIZE];
std::mutex stage_two_buffer_protector;
std::condition_variable stage_two_buffer_empty; //aka is some free space
std::condition_variable stage_two_buffer_full; //aka is some job to be done
bool stage_two_should_end = false;

void stage_two_worker_thread() {

	Matrix * current_data;

	while (!stage_two_should_end) {
		//wait for task
		//wait for buffer_empty in
		std::unique_lock<std::mutex> lk_full(stage_two_buffer_protector);
		stage_two_buffer_full.wait(lk_full, [] {return stage_two_buffer_count > 0; });

		//take data into local memory
		current_data = stage_two_buffer[stage_two_buffer_count - 1];
		stage_two_buffer[stage_two_buffer_count - 1] = nullptr;
		stage_two_buffer_count--;

		{
			std::lock_guard<std::mutex> cout(cout_mutex);
			std::cout << "Stage two: Matrix with id: " << current_data->id << " has been placed OUT of buffer." << std::endl;
			std::cout << "Stage two: Current buffer status [" << stage_two_buffer_count << "/" << STAGE2_BUFFER_SIZE << "]" << std::endl;
		}

		//notify boss
		lk_full.unlock();
		stage_two_buffer_empty.notify_one();

		//do its job
		current_data->make_triangle_form();
	}

}

void stage_two_boss_thread() {

	//first create its own threads...
	std::thread threads[STAGE2_WORKERS_COUNT];
	for (size_t i = 0; i < STAGE2_WORKERS_COUNT; i++)
		threads[i] = std::thread(stage_two_worker_thread);

	bool should_work = true;

	while (should_work) {
		//this guy has to wait for two conditions
		//wait for data ready in stage one
		std::unique_lock<std::mutex> lk_one(stage_one_data_protector);
		stage_one_data_change.wait(lk_one, [] {return stage_one_data != nullptr; });
	
		//wait for buffer_empty in
		std::unique_lock<std::mutex> lk_empty(stage_two_buffer_protector);
		stage_two_buffer_empty.wait(lk_empty, [] {return stage_two_buffer_count < STAGE2_BUFFER_SIZE; });

		//take data and put into buffer
		assert(stage_two_buffer[stage_two_buffer_count] == nullptr);
		stage_two_buffer[stage_two_buffer_count] = stage_one_data;
		stage_two_buffer_count++;
		stage_one_data = nullptr;

		{
			std::lock_guard<std::mutex> cout(cout_mutex);
			std::cout << "Stage two: Matrix with id: " << stage_two_buffer[stage_two_buffer_count - 1]->id << " has been placed INTO buffer." << std::endl;
			std::cout << "Stage two: Current buffer status [" << stage_two_buffer_count << "/" << STAGE2_BUFFER_SIZE << "]" << std::endl;
		}
		//notify worker stage one
		lk_one.unlock();
		stage_one_data_change.notify_all();

		if (stage_two_buffer_count == 0 && stage_one_should_end)
			should_work = false;

		//notify workers
		lk_empty.unlock();
		stage_two_buffer_full.notify_all();

	}
	stage_two_should_end = true;

	for (size_t i = 0; i < STAGE2_WORKERS_COUNT; i++)
		threads[i].join();


}



int main()
{
	std::thread a(stage_two_boss_thread);
	std::thread b(stage_one_thread);


	a.join();
	b.join();


}

