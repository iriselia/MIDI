/*
 * mididemo.c
 *
 *  Created on: Dec 21, 2011
 *      Author: David J. Rager
 *       Email: djrager@fourthwoods.com
 *
 * This code is hereby released into the public domain per the Creative Commons
 * Public Domain dedication.
 *
 * http://http://creativecommons.org/publicdomain/zero/1.0/
 */
#include <stdio.h>
#include <stdlib.h>

//unsigned int example1();
//unsigned int example2();
//unsigned int example3();
//unsigned int example4();
//unsigned int example5();
//unsigned int example6();
//unsigned int example7();
unsigned int example8();
unsigned int example9();
//unsigned int example10();

typedef unsigned int (*example)();

example arr[] = {
		//example1,
		//example2,
		//example3,
		//example4,
		//example5,
		//example6,
		//example7,
		example8,
		example9
};
/*
int main(int argc, char* argv[])
{
	unsigned int i;

// 	if(argc < 2)
// 		return printf("Usage: %s <example #>\n", argv[0]);

//	i = atoi(argv[1]) - 1;

	i = 8;

	if(i < sizeof(arr) / sizeof(example))
		return arr[i]();
	else
		printf("Usage: %s <example #>\n", argv[0]);

	return 0;
}

//*/