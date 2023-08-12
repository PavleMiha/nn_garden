#include <iostream>
#include <assert.h>     /* assert */
#include "value.h"

#define CHECK(x) \
	if (!(x)) \
	{ \
		std::cout << "TEST FAILED FILE " << __FILE__ << " LINE " << __LINE__ << ": " << #x << std::endl; \
		return 1; \
	}

int main() {

	Value a(2.0f);
	Value b(-3.0f);
	Value c(10.f);
	Value e = a * b;
	Value d = (e + c);
	Value f(-2.f);
	Value L = d * f;

	L.backwards();
	CHECK(a.m_value	   == 2.0f);
	CHECK(a.m_gradient == 6.f);

	CHECK(b.m_value	   == -3.f);
	CHECK(b.m_gradient == -4.f);

	CHECK(c.m_value	   == 10.f);
	CHECK(c.m_gradient == -2.f);

	CHECK(e.m_value	   == -6.f);	
	CHECK(e.m_gradient == -2.f);

	CHECK(d.m_value	   == 4.f);
	CHECK(d.m_gradient == -2.f);

	CHECK(f.m_value	   == -2.f);
	CHECK(f.m_gradient == 4.f);

	CHECK(L.m_value	   == -8.f);
	CHECK(L.m_gradient == 1.f);

	printf("TESTS SUCCEEDED\n");
	return 0;
}
