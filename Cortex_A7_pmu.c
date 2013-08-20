#include "v7_pmu.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int random_range(int max);
void pmu_start(unsigned int event0,unsigned int event1,unsigned int event2,unsigned int event3,unsigned int event4,unsigned int event5);
void pmu_stop(void);

int main ( int argc, char *argv[] ){
int matrix_size;
int i,j,k,z;



// To access CPU time
clock_t start, end;
double cpu_time_used;

  if ( argc != 2 ) {
    fputs ( "usage: $prog <square matrix size>\n", stderr );
    exit ( EXIT_FAILURE );
        }

    matrix_size = (int)strtol(argv[1],NULL,10);


printf("square matrix size = %d\n", matrix_size);


/*Using time function output for seed value*/
        unsigned int seed = (unsigned int)time(NULL);
        srand(seed);

/* Initialize square matrix with command line input value */
int a[matrix_size][matrix_size], b[matrix_size][matrix_size], c[matrix_size][matrix_size];

/* Intialize both A[][] and B[][] with random values between 0-5 and set C[][] to zero*/
        for(i=0;i<matrix_size;i++){
                for(j=0;j<matrix_size;j++){
                a[i][j]=random_range(6);
                b[i][j]=random_range(6);
                c[i][j]=0;
                }
        }

/* Multiply A[][] and B[][] and store into C[][]*/
 start = clock();



for(z=0;z<7;z++){
        if(z==0)
        pmu_start(0x01,0x02,0x03,0x04,0x05,0x06);
        if(z==1)
        pmu_start(0x07,0x08,0x09,0x0A,0x0B,0x0C);
        if(z==2)
        pmu_start(0x0D,0x0E,0x0F,0x10,0x11,0x12);
        if(z==3)
        pmu_start(0x50,0x51,0x60,0x61,0x62,0x63);
        if(z==4)
        pmu_start(0x64,0x65,0x66,0x67,0x68,0x6E);
        if(z==5)
        pmu_start(0x70,0x71,0x72,0x73,0x74,0x81);
        if(z==6)
        pmu_start(0x82,0x83,0x84,0x85,0x86,0x8A);

   for(i=0; i<matrix_size; i++)
   {
         for(j=0; j<matrix_size; j++)
         {
            for(k=0; k<matrix_size; k++)
            {
                  /* c[0][0]=a[0][0]*b[0][0]+a[0][1]*b[1][0]+a[0][2]*b[2][0]; */
                  c[i][j] += a[i][k]*b[k][j];
            }
         }
  }

        pmu_stop();

}
    end = clock();
    cpu_time_used = (end - start) / ((double) CLOCKS_PER_SEC);

printf("CPU time used = %.4lf\n",cpu_time_used);
printf("square matrix size = %d\n", matrix_size);


  return 0;
}

int random_range(int max){
    return ((rand()%(max-1)) +1);
}



void pmu_start(unsigned int event0,unsigned int event1,unsigned int event2,unsigned int event3,unsigned int event4,unsigned int event5){

  enable_pmu();              // Enable the PMU
  reset_ccnt();              // Reset the CCNT (cycle counter)
  reset_pmn();               // Reset the configurable counters
  pmn_config(0, event0);       // Configure counter 0 to count event code 0x03
  pmn_config(1, event1);       // Configure counter 1 to count event code 0x03
  pmn_config(2, event2);       // Configure counter 2 to count event code 0x03
  pmn_config(3, event3);       // Configure counter 3 to count event code 0x03
  pmn_config(4, event4);       // Configure counter 4 to count event code 0x03
  pmn_config(5, event5);       // Configure counter 5 to count event code 0x03

  enable_ccnt();             // Enable CCNT
  enable_pmn(0);             // Enable counter
  enable_pmn(1);             // Enable counter
  enable_pmn(2);             // Enable counter
  enable_pmn(3);             // Enable counter
  enable_pmn(4);             // Enable counter
  enable_pmn(5);             // Enable counter

  printf("CountEvent0=0x%x,CountEvent1=0x%x,CountEvent2=0x%x,CountEvent3=0x%x,CountEvent4=0x%x,CountEvent5=0x%x\n", event0,event1,event2,event3,event4,event5);
}

void pmu_stop(void){
  unsigned int cycle_count, overflow, counter0, counter1, counter2, counter3, counter4, counter5;

  disable_ccnt();            // Stop CCNT
  disable_pmn(0);            // Stop counter 0
  disable_pmn(1);            // Stop counter 1
  disable_pmn(2);            // Stop counter 2
  disable_pmn(3);            // Stop counter 3
  disable_pmn(4);            // Stop counter 4
  disable_pmn(5);            // Stop counter 5

  counter0    = read_pmn(0); // Read counter 0
  counter1    = read_pmn(1); // Read counter 1
  counter2    = read_pmn(2); // Read counter 2
  counter3    = read_pmn(3); // Read counter 3
  counter4    = read_pmn(4); // Read counter 4
  counter5    = read_pmn(5); // Read counter 5

  cycle_count = read_ccnt(); // Read CCNT
  overflow=read_flags();        //Check for overflow flag

  printf("Counter0=%d,Counter1=%d,Counter2=%d,Counter3=%d,Counter4=%d,Counter5=%d\n", counter0, counter1,counter2,counter3,counter4,counter5);
  printf("Overflow flag: = %d, Cycle Count: = %d \n\n", overflow,cycle_count);
}
