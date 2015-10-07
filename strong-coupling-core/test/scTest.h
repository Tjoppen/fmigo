#define SCTEST_EQUAL_MESSAGE(a,b,m) \
if(a!=b){ \
  fprintf(stderr,"%s\n",m); \
  return 1; \
}
