#include "Matrix.h"

#include <iostream> 
#include <iomanip>
#include <assert.h>




Matrix::Matrix()
{
	//generate space on heap
	matrix_data = new double[MATRIX_DIM_SQ];
	vector_data = new double[MATRIX_DIM];


	//fill matrix with random data
	for (size_t i = 0; i < MATRIX_DIM_SQ; i++)
		matrix_data[i] = distribution(gen);

	for (size_t i = 0; i < MATRIX_DIM; i++)
		vector_data[i] = distribution(gen);
}

Matrix::Matrix(int test)
{
	assert(MATRIX_DIM == 3);

	//generate space on heap
	matrix_data = new double[MATRIX_DIM_SQ];
	vector_data = new double[MATRIX_DIM];

	double data[] = { 1,2,-4, 2,1, -6, 4, -1, -12 };
	double x[] = { 5,8,13 };

	//fill matrix with random data
	for (size_t i = 0; i < MATRIX_DIM_SQ; i++)
		matrix_data[i] = data[i];

	for (size_t i = 0; i < MATRIX_DIM; i++)
		vector_data[i] = x[i];
}

Matrix::~Matrix()
{
	delete [] matrix_data;
	delete [] vector_data;
}

void Matrix::make_triangle_form()
{
	for (int j = 0; j < MATRIX_DIM; j++) {
		for (int i = j; i < MATRIX_DIM; i++) {
			if (i != j) {
				double b = matrix_data[i * MATRIX_DIM + j] / matrix_data[j * MATRIX_DIM + j];
				for (int k = 0; k < MATRIX_DIM; k++) {
					matrix_data[i * MATRIX_DIM + k] = matrix_data[i * MATRIX_DIM + k] - b * matrix_data[j * MATRIX_DIM + k];
				}
				vector_data[i] = vector_data[i] - b * vector_data[j];
			}
		}
	}
}



std::ostream & operator<<(std::ostream & stream, const Matrix & matrix)
{
	auto print_fixed = [&stream](double value) {
		if (value > 0)
			stream << " ";
		if (value > -10 && value < 10)
			stream << " ";
		stream << value;
	};

	stream << std::setprecision(2) << std::fixed;

	for (size_t x = 0; x < MATRIX_DIM; x++)
	{
		stream << "[";
		for (size_t y = 0; y < MATRIX_DIM; y++)
		{
			int index = x * MATRIX_DIM + y;
			print_fixed(matrix.matrix_data[index]);
			stream << " ";
		}

		if (MATRIX_DIM / 2 == x) {
			stream << "] = [";
		}
		else {
			stream << "]   [";
		}


		print_fixed(matrix.vector_data[x]);
		stream << "]" << std::endl;


	}

	return stream;
}
