#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <snmp/libsnmp.h>
#if 0
#include "asn1.h"
#include "parse.h"
#endif

SNMP_NAMESPACE
	extern int init_mib(void);
SNMP_NAMESPACE_END


void main(int argc, char **argv)
{
  init_mib();
  printf("MIB Initialized\n");
}
// $Log: $
