#pragma once
#include "Constants.h"


#include <random>



class Matrix
{
public:
	Matrix();
	~Matrix();

	friend std::ostream& operator<< (std::ostream& stream, const Matrix& matrix);

	
private:
	//data
	double * matrix_data;
	double * vector_data;


	//random
	std::mt19937 gen;
	std::uniform_real_distribution<> distribution{-100, 100};



};

