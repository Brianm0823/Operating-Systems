%{
#include <stdio.h>
#include <iostream>
#include "scanner.h"
using namespace std;
%}

%%
[ \t\r\n]      ;
input_var	return INPUT_VAR;
;		return SEMICOLON;
internal_var	return INTERNAL_VAR;
write\(		return WRITE;
[A-Za-z]+[0-9]+	return PROCESS;
->		return ASSIGN;
[-][0-9]+	return NUM;
[-+*/]		return OP;
[a-zA-z]	return LETTER;
\.		return PERIOD;
[0-9]+		return NUM;
.		;

%%

int yywrap(void) 
{
	return 1;
}

