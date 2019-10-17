%{
#include <stdio.h>
#include <iostream>
#include "scanner.h"
using namespace std;
%}

%%
[ \t\r\n]      ;
reserve		return RESERVE;
cancel		return CANCEL;
check		return CHECK;
pay			return PAY;
deadline 	return DEADLINE;
end			return END;
[A-Za-z]+[_][0-9]+[:]	return CUSTOMER;
[A-Za-z]+	return NAME;
\/			return SLASH;
\(			return OPENPARENTHESIS;
-			return DASH;
,			return COMMA;
\) 			return CLOSEDPARENTHESIS;
[0-9]+		return NUM;
.		;

%%

int yywrap(void) 
{
	return 1;
}
