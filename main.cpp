#include <stdio.h>
#include <functional.hpp>

void MyFunction(int a)
{
	printf("%d\n",a);
}

int main()
{
	using namespace std::placeholders;  // for std::placeholders::_1

	std::function<void(int)>     func=std::bind(&MyFunction,_1);
	func(5);

	return 0;
}
