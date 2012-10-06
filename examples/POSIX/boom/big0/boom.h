/*
   boom.h
*/

extern int printf(char *fmt,...);

#define TASK(n)\
{\
unsigned long i=0;\
unsigned long j=1;\
unsigned long k=2;\
unsigned long l=3;\
unsigned long m=4;\
   for(;i<10000;i++) {\
      switch(i) {\
      case 0 :\
         if(j==56) {\
            switch(m) {\
            case 789 :\
               for(;l<10000;l++) {\
                  switch(k) {\
                  case 45 :\
                     function1(n,i);\
                     function2(n,j);\
                     function3(n,k);\
                     break;\
                  case 47 :\
                     function1(n,i);\
                     function2(n,j);\
                     function3(n,k);\
                     break;\
                  case 49 :\
                     function1(n,i);\
                     function2(n,j);\
                     function3(n,k);\
                     break;\
                  case 567 :\
                     function1(n,i);\
                     function2(n,j);\
                     function3(n,k);\
                     break;\
                  default:\
                     break;\
                  }\
               }\
               break;\
            case 798 :\
               for(;m<10000;m++) {\
                  switch(k) {\
                  case 45 :\
                     function1(n,m);\
                     function2(n,k);\
                     function3(n,j);\
                     break;\
                  case 47 :\
                     function1(n,m);\
                     function2(n,j);\
                     function3(n,k);\
                     break;\
                  case 49 :\
                     function1(n,k);\
                     function2(n,l);\
                     function3(n,k);\
                     break;\
                  case 567 :\
                     function1(n,i);\
                     function2(n,j);\
                     function3(n,k);\
                     break;\
                  default:\
                     break;\
                  }\
               }\
               break;\
            default:\
               break;\
            }\
         }\
         else {\
            function1(n,i);\
            function2(n,j);\
         }\
         break;\
      case 1 :\
         for(;i;) {\
            if(j==789) {\
               function1(n,i);\
               function2(n,j);\
               function3(n,k);\
            }\
            else {\
               function2(n,l);\
               function3(n,m);\
            }\
         }\
         break;\
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
