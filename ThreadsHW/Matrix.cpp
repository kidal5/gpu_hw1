#include "Matrix.h"

#include <iomanip> 



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

Matrix::~Matrix()
{
	delete[] matrix_data;
	delete[] vector_data;

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
