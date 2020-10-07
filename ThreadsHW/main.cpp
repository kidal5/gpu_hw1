
#include "Matrix.h"
#include <iostream>

void matrix_print_test() {
	Matrix m;
	std::cout << m << std::endl << std::endl;

	m.make_triangle_form();
	std::cout << m;


}

int main()
{
	matrix_print_test();
}

