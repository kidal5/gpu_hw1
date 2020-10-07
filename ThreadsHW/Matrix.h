#pragma once
#include "Constants.h"


#include <random>


using mat = std::vector<std::vector<double>>;

enum class MatrixSolveType {
	ONE,
	NONE,
	INF
};

class Matrix
{
public:
	Matrix();
	Matrix(int test);

	void make_triangle_form();
	void compute_ranks();
	void solve();

	friend std::ostream& operator<< (std::ostream& stream, const Matrix& matrix);

private:
	int compute_rank_inner(mat A, bool extended);
	
private:
	//data
	mat data = mat(MATRIX_DIM, std::vector<double>(MATRIX_DIM + 1));
	std::vector<double> solution;

	//random
	std::mt19937 gen;
	std::uniform_real_distribution<> distribution{-100, 100};

	//ranks
	int rank = -1;
	int rank_extended = -1;
	MatrixSolveType type;

};

