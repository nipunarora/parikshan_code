/* 
 * 
 * Filename: loop_queue_sim.c
 * Author: Nipun Arora
 * Created: Sun Aug 23 22:06:05 2015 (-0400)
 * URL: http://www.nipunarora.net 
 * 
 * Description: 
 */

//----- Include files ----------------------------------------------------------
#include <stdio.h>              // Needed for printf()
#include <stdlib.h>             // Needed for exit() and rand()
#include <math.h>               // Needed for log()

//----- Constants --------------------------------------------------------------
#define SIM_TIME   1.0e6        // Simulation time
#define ARR_TIME   1.25         // Mean time between arrivals
#define SERV_TIME  1.00         // Mean service time

//----- Function prototypes ----------------------------------------------------
double rand_val(int seed);      // RNG for unif(0,1)
double exponential(double x);   // Generate exponential RV with mean x

//===== Main program ===========================================================
int main(int argc, char *argv[])
{

  if(argc != 6)
    {
      printf(" Please provide the following arguments: simulation time, mean time between arrivals, mean service time, initial buf size, step buf size \n");
      exit(1);
    }

  unsigned int buf = atoi(argv[4]);
  unsigned int step = atoi(argv[5]);
  int cnt;
  
  for(cnt = 0; cnt <10; cnt++)
    {

      double end_time = strtod(argv[1], NULL);
      double Ta = strtod(argv[2], NULL);
      double Ts = strtod(argv[3], NULL);
  
      //double   end_time = SIM_TIME; // Total time to simulate
      //double   Ta = ARR_TIME;       // Mean time between arrivals
      //double   Ts = SERV_TIME;      // Mean service time
      double   time = 0.0;          // Simulation time
      double   t1 = 0.0;            // Time for next event #1 (arrival)
      double   t2 = SIM_TIME;       // Time for next event #2 (departure)
      unsigned int n = 0;           // Number of customers in the system
      unsigned int c = 0;           // Number of service completions
      double   b = 0.0;             // Total busy time
      double   s = 0.0;             // Area of number of customers in system
      double   tn = time;           // Variable for "last event time"
      double   tb;                  // Variable for "last start of busy time"
      double   x;                   // Throughput
      double   u;                   // Utilization
      double   l;                   // Mean number in the system
      double   w;                   // Mean residence time

      // Seed the RNG
      rand_val(1);

      unsigned int max_buf = 0;
      // Main simulation loop
      while (time < end_time)
	{
	  if (t1 < t2)                // *** Event #1 (arrival) ***
	    {
	      time = t1;
	      s = s + n * (time - tn);  // Update area under "s" curve
	      n++;
	      tn = time;                // tn = "last event time" for next event
	      t1 = time + exponential(Ta);
	      if (n == 1)
		{
		  tb = time;              // Set "last start of busy time"
		  t2 = time + exponential(Ts);
		}
	    }
	  else                        // *** Event #2 (departure) ***
	    {
	      time = t2;
	      s = s + n * (time - tn);  // Update area under "s" curve
	      n--;
	      tn = time;                // tn = "last event time" for next event
	      c++;                      // Increment number of completions
	      if (n > 0)
		t2 = time + exponential(Ts);
	      else
		{
		  t2 = SIM_TIME;
		  b = b + time - tb;      // Update busy time sum if empty
		}
	    }

	  if(max_buf < n)
	    max_buf = n;
	  
	  if(n>buf){
	    break;
	  }	  
	}

      printf("{\n");
      printf("\"P-Buf\": %d,\n",buf);
      printf("\"buf-size\": %d,\n",n);
      printf("\"max-buf\": %d,\n",max_buf);
      printf("\"hitting\": %3.2f\n",time);
      printf("}\n");

	/*
	// End of simulation so update busy time sum
	b = b + time - tb;

	// Compute outputs
	x = c / time;   // Compute throughput rate
	u = b / time;   // Compute server utilization
	l = s / time;   // Compute mean number in system
	w = l / x;      // Compute mean residence or system time
     
	// Output results
	printf("=============================================================== \n");
	printf("=            *** Results from M/M/1 simulation ***            = \n");
	printf("=============================================================== \n");
	printf("=  Total simulated time         = %3.4f sec   \n", time);
	printf("=============================================================== \n");
	printf("=  INPUTS:                                    \n");
	printf("=    Mean time between arrivals = %f sec      \n", Ta);
	printf("=    Mean service time          = %f sec      \n", Ts);
	printf("=============================================================== \n");
	printf("=  OUTPUTS:                                   \n");
	printf("=    Number of completions      = %ld cust    \n", c);
	printf("=    Throughput rate            = %f cust/sec \n", x);
	printf("=    Server utilization         = %f %%       \n", 100.0 * u);
	printf("=    Mean number in system      = %f cust     \n", l);
	printf("=    Mean residence time        = %f sec      \n", w);
	printf("=============================================================== \n");
      */
      buf = buf + 1000;
    }
  return(0);
}

//=========================================================================
//= Multiplicative LCG for generating uniform(0.0, 1.0) random numbers    =
//=   - x_n = 7^5*x_(n-1)mod(2^31 - 1)                                    =
//=   - With x seeded to 1 the 10000th x value should be 1043618065       =
//=   - From R. Jain, "The Art of Computer Systems Performance Analysis," =
//=     John Wiley & Sons, 1991. (Page 443, Figure 26.2)                  =
//=   - Seed the RNG if seed > 0, return a unif(0,1) if seed == 0         =
//=========================================================================
double rand_val(int seed)
{
  const long  a =      16807;  // Multiplier
  const long  m = 2147483647;  // Modulus
  const long  q =     127773;  // m div a
  const long  r =       2836;  // m mod a
  static long x;               // Random int value (seed is set to 1)
  long        x_div_q;         // x divided by q
  long        x_mod_q;         // x modulo q
  long        x_new;           // New x value

  // Seed the RNG
  if (seed != 0) x = seed;

  // RNG using integer arithmetic
  x_div_q = x / q;
  x_mod_q = x % q;
  x_new = (a * x_mod_q) - (r * x_div_q);
  if (x_new > 0)
    x = x_new;
  else
    x = x_new + m;

  // Return a random value between 0.0 and 1.0
  return((double) x / m);
}

//==============================================================================
//=  Function to generate exponentially distributed RVs using inverse method   =
//=    - Input:  x (mean value of distribution)                                =
//=    - Output: Returns with exponential RV                                   =
//==============================================================================
double exponential(double x)
{
  double z;                     // Uniform random number from 0 to 1

  // Pull a uniform RV (0 < z < 1)
    do
      {
	z = rand_val(0);
      }
    while ((z == 0) || (z == 1));

    return(-x * log(z));
}
