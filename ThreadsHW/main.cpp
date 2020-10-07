
#include "Matrix.h"
#include <iostream>

void matrix_print_test() {
	Matrix m(3);
	m.make_triangle_form();
	m.solve();
	m.write_to_file();
}

int main()
{
	matrix_print_test();
}

