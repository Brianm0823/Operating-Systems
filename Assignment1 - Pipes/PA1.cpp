#include <iostream>
#include <fstream>
#include <unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include <stdlib.h>
#include <queue>
#include <string>
#include <sstream>
#include <ctype.h>
//used for lex analyzer//
#include "scanner.h"
extern int yylex();
extern char *yytext;
extern FILE *yyin;
/////////////////////////

using namespace std;


int main(int argc, char * argv[]) {
	if(argc != 3) { //ends program if incorrect arguments are used
		cout << "Usage example: PA1 sample_commands.txt sample_data.txt" << endl;
		return -1;
	}

	string commandsFile = argv[1]; //stores commands file name from run arguement
	string inputDataFile = argv[2]; //stores input data file name from run argument

	int token; //stores identifier for lex analyzer
	string input_variables[10][2]; //stores all the input variables letters and corresponding number
	int inputvarCount = 0; //number of input variables
	string internal_variables[10][20]; //stores internal variable names and every input variable/operator to be sent to that process
	int amountToProcess[10] = {0,0,0,0,0,0,0,0,0,0}; //array that stores the amount of numbers and operators to be sent to each process
	int internalvarCount = 0; // number of internal variables
	string write_parameters[20]; // stores the parameters to be used in the final write function
	int parametersCount = 0; // number of write parameters
	string tempOp;
	int pipeCounter = 0;

	//INPUT PARSING **********************************************************************************************************************
	yyin = fopen(commandsFile.c_str(), "r"); //feeds commandsFile to lex analyzer
	token = yylex(); //gets first token from commandsFile using lex
	while(token) { //will keep the loop running until there's nothing else to analyze
	if(token == 1) { //if it reads "input_var" from commandsFile it will run the following code
		token = yylex(); //gets next token and runs in a loop until a semicolon is reached, which marks end of input variables
		while(token != 8) {
			input_variables[inputvarCount][0] = yytext; //stores every single input variable name in an array
			inputvarCount++;
			token = yylex();
		}
	}
	if(token == 3) { //if it reads "internal_var" from commandsFile it will run the following code
		token = yylex();
		while(token != 8) { //gets next token and runs in a loop until a semicolon is reached, which marks end of internal variables
			internal_variables[internalvarCount][0] = yytext; //stores every single internal variable name in an array
			internalvarCount++;
			token = yylex();
		}
	}
	if(token == 7) { //if it reads "write" from the commandsFile it will run the following code
		token = yylex();
		while(token!=9) { //gets next token and runs in a loop until a period is reached, which marks end of write arguments
			write_parameters[parametersCount] = yytext; //stores every single write argument in an array
			parametersCount++;
			token = yylex();
		}
		yyin = fopen(inputDataFile.c_str(), "r"); //feeds input data file to lex analyzer
		token = yylex(); //gets the first token (input number)
		int counter = 0;
		while(token) { //will run the loop until yylex() returns 0, which signals end of file
			input_variables[counter][1] = yytext; //stores each number with it's corresponding variable name
			counter++;
			token = yylex();
		}
	}
	if(token == 2 || token == 4) { //if it reads an input variable or internal variable
		string temp;
		temp = yytext; //stores the variable in temp
		yylex(); // reads a "->" which signals the use of a pipe, so increment pipe counter
		pipeCounter++;
		token = yylex();
		for(int i = 0; i < internalvarCount; i++) {
			if(internal_variables[i][0] == yytext) {
				if(tempOp != "") {
					int x = amountToProcess[i];
					internal_variables[i][x+1] = tempOp;
					amountToProcess[i] = amountToProcess[i] + 1;
					//cout << "added " << tempOp << " to " << yytext << endl;
					tempOp = "";
				}
				int x = amountToProcess[i];
				internal_variables[i][x+1] = temp;
				amountToProcess[i] = amountToProcess[i] + 1;
				//cout << "added " << temp << " to " << yytext << endl;
			}
		}
	}
	if(token == 6) {
		tempOp = yytext;
		//cout << "tempOp:" << tempOp << endl;
	}
	token = yylex();
	}
	//END OF INPUT PARSING *************************************************************************************************************


	/*
	//input variable cout testing
	cout << "input variables array: " ;
	for(int i = 0; i < inputvarCount; i++) {
					cout << input_variables[i][0] << "=" << input_variables[i][1] << " ";
		}
	cout << endl;
	//***************************
	//internal variable cout testing
	cout << "internal variable array: " << endl;
	for(int i = 0; i < internalvarCount; i++) {
					for(int j = 0; j < amountToProcess[i] + 1; j++) {
						cout << internal_variables[i][j] << " ";
					}
					cout << endl;
		}
	cout << endl;
	*/

	pid_t pid[10]; //creates array of pid's with a maximum of 10 (for each fork)
	int variablesToProcess[10][2]; //number of variables that will go to each process including operators
	for(int i = 0; i < internalvarCount; i++) {
		variablesToProcess[i][1] = (amountToProcess[i] +1 )/2;
	}

	string final_values[internalvarCount][2]; //Array that holds the final value of every process
	for(int i = 0; i < internalvarCount; i++) {
		string temp = internal_variables[i][0];
		final_values[i][0] = temp;
	}


	//creates a 2d array of file descriptors. internalvarCount is # of child processes
	//pipeCountecoutr is how many pipes I need per child process * 2 (write and read for each)
	int fd[internalvarCount][pipeCounter*2]; // used to send values to child processes for processing
	int fd2[internalvarCount][2]; // used to send the result from child process to parent
	int fd3[internalvarCount][internalvarCount*2]; //used to send process results to children that need them to process their result

	for (int i = 0; i < internalvarCount; i++) {
		for(int j = 0; j < amountToProcess[i]; j++)
		{
			if((pipe(fd[i]+2*j) == -1) ) { //creates a 2d array of pipes so each process has its own pipes to use
				perror("Error creating pipe");
				exit(0);
			}
		}
	}

	for (int i = 0; i < internalvarCount; i++) {
			for(int j = 0; j < internalvarCount; j++)
			{
				if((pipe(fd3[i]+2*j) == -1) ) { //same thing as above
					perror("Error creating pipe");
					exit(0);
				}
			}
		}

	for(int i = 0; i < internalvarCount; i++) {
		if((pipe(fd2[i]) == -1) ) { //same thing as above
			perror("Error creating pipe");
			exit(0);
		}
	}


	queue <string> calculate; // will hold the numbers and operators in the child processes after reading from pipes so it can do arithmetic in order
	queue <string> tempCalculate;



	//loop that creates internalvarCount amount of concurrent child processes
	for(int i = 0; i < internalvarCount; i++) {
		pid[i] = fork();

		if(pid[i] < 0) {
			perror("Bad fork");
		}
		else if(pid[i] == 0) { //is child

			//reads from the pipes and gets the values and operators needed to do the math
			for(int j = 0; j < amountToProcess[i]; j++) {
				char temp1[80];
				close(fd[i][2*j+1]);
				read(fd[i][2*j], temp1, 80 );
				close(fd[i][2*j]);
				tempCalculate.push(temp1); //stores them in temp queue
			}


			while(!tempCalculate.empty()) {
				string t = tempCalculate.front();
				if(t[0] == 'p'){ //if any process needs the value of another process before being able to do
					char temp2[80]; //the math, it will read them from a pipe
					int needed = t[1] - 48;
					close(fd3[i][needed*2+1]);
					read(fd3[i][needed*2], temp2, 80 );
					close(fd3[i][needed*2]);
					calculate.push(temp2);
				}
				else {
					calculate.push(t);
				}
				tempCalculate.pop();
			}
			string front;
			int final = 0;
			int temp1;
			int temp2;
			int ready = 0;
			string op;
			while(!calculate.empty()) { //will keep looping until everything in queue is taken care of
				front = calculate.front();
				if(front == "+" || front == "-" || front == "/" || front == "*") { //if more than 2 values have to be processed, it will take over from here
					op = front; //stores operator
					calculate.pop();
					front = calculate.front(); //gets next number and does the correct math
					temp1 = atoi(front.c_str());
					if(op == "+")
						final = final + temp1;
					else if(op == "-")
						final = final - temp1;
					else if(op == "/")
						final = final / temp1;
					else if(op == "*")
						final = final * temp1;
						ready = 0;

				}
				else if(ready == 0) { //will run as the first part in every child process
					temp1 = atoi(front.c_str()); //stores first value of queue
					ready++; //increments ready so next block runs in the next iteration of the loop
					calculate.pop();
					op = calculate.front(); //stores the operator following
				}
				else if (ready == 1){
					temp2 = atoi(front.c_str()); //stores next value and does the correct math
					if(op == "+")
						final = temp1 + temp2;
					else if(op == "-")
						final = temp1 - temp2;
					else if(op == "/")
						final = temp1 / temp2;
					else if(op == "*")
						final = temp1 * temp2;
					ready = 0;
				}
				calculate.pop();
			}
			close(fd2[i][0]);
			write(fd2[i][1], &final, sizeof(final)); //writes the final value of each child to a pipe so parent can read it
			close(fd2[i][1]);
			exit(0);
		}
		else { //is parent
			int result;
			for(int j = 0; j < amountToProcess[i]; j++) { //code to send every child process the values it needs
				string send = internal_variables[i][j+1];
				if(send.length() == 1) {
					if(isalpha(send[0])) {
						string t = send;
						for(int x = 0; x < inputvarCount; x++) {
							if(input_variables[x][0] == t) {
								send = input_variables[x][1]; //stores the value to be sent
							}
						}
					}
				}
				else { //this will run if a child needs the final value of another child to get its result
					int needed = send[1] - 48;
					string sendstring = final_values[needed][1];
					close(fd3[i][needed*2]);
					write(fd3[i][needed*2+1], sendstring.c_str(), sendstring.length()+1);
					close(fd3[i][needed*2+1]);
				}
				//sends the stored value to the child processes through a pipe
				close(fd[i][2*j]);
				write(fd[i][2*j+1], send.c_str(), send.length()+1);
				close(fd[i][2*j+1]);
			}
			close(fd2[i][1]);
			read(fd2[i][0], &result, sizeof(result)); //reads the final value of child processes
			close(fd2[i][0]);
			ostringstream tempResult;
			tempResult << result;
			final_values[i][1] = tempResult.str(); //stores the value of each process in a final array

		}
	}

	for (int i = 0; i < internalvarCount; i++) {
        wait(NULL); //parent waits for child processes to finish
    }
	//prints input variables and corresponding value
	for(int i = 0; i < inputvarCount; i++) {
		cout << input_variables[i][0] << " = " << input_variables[i][1] << endl;
	}
	//prints internal variables and corresponding values
	for(int i = 0; i < internalvarCount; i++) {
		cout << final_values[i][0] << " = " << final_values[i][1] << endl;
	}
	return 0;
}
