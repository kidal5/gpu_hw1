#include "Matrix.h"

#include <string>
#include <vector>
#include <fstream>
#include <iostream> 
#include <iomanip>
#include <assert.h>
#include <mutex>
#include <filesystem>

namespace {
	std::ostream& operator << (std::ostream& os, const MatrixSolveType& type)
	{
		switch (type) {
		case MatrixSolveType::ONE:
			os << "ONE";
			break;
		case MatrixSolveType::NONE:
			os << "NONE";
			break;
		case MatrixSolveType::INF:
			os << "INF";
			break;
		}
		return os;
	}
}

Matrix::Matrix(int id) : id(id)
{
	for (size_t x = 0; x < MATRIX_DIM; x++)
	{
		for (size_t y = 0; y < MATRIX_DIM; y++)
			data[x][y] = distribution(gen);
		data[x][MATRIX_DIM] = distribution(gen);
	}

	data_copy = data;
}

Matrix::Matrix(int id, int testset) : id(id)
{
	assert(MATRIX_DIM == 3);

	//set zero
	double data_0[] = { 1,2,-4, 2,1, -6, 4, -1, -12 };
	double x_0[] = { 5,8,13 };

	//set one
	double data_1[] = { 1,0,0, 0,1, 0, 0, 0, -0 };
	double x_1[] = { 5,8,13 };

	//uzlabina2.aspone.cz/KralfrobeniovaVeta.aspx
	double data_2[] = { 1,2,5, 0,1, 5, 0, 0, 1 };
	double x_2[] = { 4,2,1 };

	double data_3[] = { 1,8,1, 0,5, 3, 0, 0, 0 };
	double x_3[] = { 10,2,4 };

	double data_4[] = { 1,4,5, 0,1, 2, 0, 0, 0 };
	double x_4[] = { 5,3,0 };

	double * _data;
	double * _x;

	switch (testset) {
	case 0:
		_data = data_0;
		_x = x_0;
		break;
	case 1:
		_data = data_1;
		_x = x_1;
		break;
	case 2:
		_data = data_2;
		_x = x_2;
		break;
	case 3:
		_data = data_3;
		_x = x_3;
		break;
	case 4:
		_data = data_4;
		_x = x_4;
		break;
	default:
		throw std::exception("test set out of range");
	}

	for (size_t x = 0; x < MATRIX_DIM; x++)
	{
		for (size_t y = 0; y < MATRIX_DIM; y++)
			data[x][y] = _data[x * MATRIX_DIM + y];
		data[x][MATRIX_DIM] = _x[x];
	}

	data_copy = data;
}

void Matrix::make_triangle_form()
{
	for (int j = 0; j < MATRIX_DIM; j++) {
		for (int i = j; i < MATRIX_DIM; i++) {
			if (i != j) {
				double b = data[i][j] / data[j][j];
				for (int k = 0; k < MATRIX_DIM + 1; k++) {
					data[i][k] = data[i][k] - b * data[j][k];
				}
			}
		}
	}
}

void Matrix::print_matrix(std::ostream & stream, const mat & matrix) const
{
	auto print_fixed = [&stream](double value) {
		stream << std::setw(8) << std::setfill(' ') << value;
	};

	stream << std::setprecision(2) << std::fixed;

	for (size_t x = 0; x < MATRIX_DIM; x++)
	{
		stream << "[";
		for (size_t y = 0; y < MATRIX_DIM; y++)
		{
			print_fixed(matrix[x][y]);
			stream << " ";
		}

		if (MATRIX_DIM / 2 == x) {
			stream << "] = [";
		}
		else {
			stream << "]   [";
		}


		print_fixed(matrix[x][MATRIX_DIM]);
		stream << "]" << std::endl;
	}
}

void Matrix::compute_ranks()
{
	rank = compute_rank_inner(data, false);
	rank_extended = compute_rank_inner(data, true);

	//one solution  when rank == rank_extended
	//no  solution  when rank != rank_extended && rank_extended == MATRIX_DIM
	//inf solutions when rank == rank_extended && rank_extended < MATRIX_DIM
	if (rank != rank_extended) {
		type = MatrixSolveType::NONE;
	}
	else {
		type = rank == MATRIX_DIM ? MatrixSolveType::ONE : MatrixSolveType::INF;
	}

}

void Matrix::solve()
{
	compute_ranks();

	if (type == MatrixSolveType::NONE) return;
	if (type == MatrixSolveType::INF) return;

	//www.tutorialspoint.com/cplusplus-program-to-implement-gauss-jordan-elimination
	for (int j = 0; j < MATRIX_DIM; j++) {
		for (int i = 0; i < MATRIX_DIM; i++) {
			if (i != j) {
				double b = data[i][j] / data[j][j];
				for (int k = i; k < MATRIX_DIM + 1; k++) {
					data[i][k] = data[i][k] - b * data[j][k];
				}
			}
		}
	}

	for (int x = 0; x < MATRIX_DIM; x++) {
		solution.push_back(data[x][MATRIX_DIM] / data[x][x]);
	}

}

void Matrix::write_to_file()
{
	namespace fs = std::experimental::filesystem;

	if (!fs::exists("results")) {
		fs::create_directory("results");
	}

	std::string fname = "results/matrix_" + std::to_string(id) + ".txt";

	std::ofstream myfile(fname);
	myfile << "Source matrix: " << std::endl;
	print_matrix(myfile, data_copy);
	myfile << std::endl;

	myfile << "Rank of A: " << rank << std::endl;
	myfile << "Rank of AB: " << rank_extended << std::endl;

	myfile << "Solution type: " << type << std::endl;

	if (type == MatrixSolveType::ONE) {
		myfile << "Solution: [";
		for (const auto & value : solution) {
			myfile << std::setw(8) << std::setfill(' ') << value;
			myfile << ", ";
		}
		myfile << "]";
	}

	myfile.close();
}

int Matrix::compute_rank_inner(mat A, bool extended) {
	//taken from cp-algorithms.com/linear_algebra/rank-matrix.html

	const double EPS = 1E-9;
	int n = A.size();
	int m = A[0].size();
	if (!extended)
		m--;

	int rank = 0;
	std::vector<bool> row_selected(n, false);
	for (int i = 0; i < m; ++i) {
		int j;
		for (j = 0; j < n; ++j) {
			if (!row_selected[j] && abs(A[j][i]) > EPS)
				break;
		}

		if (j != n) {
			++rank;
			row_selected[j] = true;
			for (int p = i + 1; p < m; ++p)
				A[j][p] /= A[j][i];
			for (int k = 0; k < n; ++k) {
				if (k != j && abs(A[k][i]) > EPS) {
					for (int p = i + 1; p < m; ++p)
						A[k][p] -= A[j][p] * A[k][i];
				}
			}
		}
	}
	return rank;
}



std::ostream & operator<<(std::ostream & stream, const Matrix & matrix)
{
	matrix.print_matrix(stream, matrix.data);
	return stream;
}
