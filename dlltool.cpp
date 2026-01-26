#include <Windows.h>

struct R1
{
	int x = 0;
	int y = 0;
};

extern "C" __declspec(dllexport) R1 calc1(int a, int b) 
{
	R1 result;
	result.x = a + b;
	result.y = a - b;
	return result;
}