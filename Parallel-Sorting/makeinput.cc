#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fstream>
using namespace std;

int main(int argc, char *argv[])
{
  if (argc != 3) {
  	fprintf(stderr, "Syntax: %s three inputs\n", argv[0]);
  	exit(1);
  }
  char *filename = argv[2];
  ofstream outfile(filename);
  srand(time(0));
  for (int i=0; i<atoi(argv[1]); i++) {
  	long long v = rand();
  	for (int j=0; j<3; j++)
  	  v = (v<<16)+rand(); // bitwise operation
  	outfile << v << '\n';
   // printf("%lld\n", v);
  }

  return 0;
}
