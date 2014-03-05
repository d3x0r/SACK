#include <sack_types.h>
#include <sharemem.h>
#include <pssql.h>

#include "login.h"

static struct {
	INDEX nSession;
 	char real_bingoday[24];
   INDEX iSessionID;
}l;

