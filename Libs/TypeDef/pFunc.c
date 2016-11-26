typedef void (*PTRFUNC)(int , int);

PTRFUNC pfunc_1;
PTRFUNC pfuncs[2];

void foo(int a, int b)
{
	printf("%d, %d", a, b);
}

void bar(int a, int b)
{
	printf("%d, %d", a, b);
}

void main()
{
	pfunc_1 = foo;
	(*pfunc_1)(1, 2);

	pfuncs[0] = foo;
	pfuncs[1] = bar;

	(*pfuncs[0])(1, 2);
	(*pfuncs[1])(2, 1);
}
