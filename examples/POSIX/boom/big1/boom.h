/*
   boom.h
*/

extern int printf(char *fmt,...);

#define TASK(n)\
{\
unsigned long i=0;\
unsigned long j=0;\
unsigned long k=0;\
   for(;i<10000;i++,j++,k++) {\
      if(j==789) {\
         function1(n,i);\
         function2(n,j);\
         function3(n,k);\
      }\
   }\
}

static void function1(tn,p0)
int tn;
unsigned long p0;
{
   printf("function1(%d,%lu)\n",tn,p0);
}

static void function2(tn,p0)
int tn;
unsigned long p0;
{
   printf("function2(%d,%lu)\n",tn,p0);
}

static void function3(tn,p0)
int tn;
unsigned long p0;
{
   printf("function3(%d,%lu)\n",tn,p0);
}
