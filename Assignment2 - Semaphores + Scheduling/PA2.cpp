#include <iostream> 
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <queue>
#include <time.h>
#include <string.h>
#include <algorithm>
#include <iomanip>
#include "scanner.h"

//required for yylex input reading
extern int yylex();
extern char *yytext;
extern FILE *yyin;

using namespace std;

int sem_signal(int, int); //function that does the required processes for signaling
int sem_wait(int, int); //function that does the required processes for waiting

struct Room {
	int date_booked[12][31][3]; //[MONTHS][DAYS][0 = CUSTOMER NUMBER, 1 = PAID, 2 = customer name (maps to other array)]
	int room_counter;
};

//stuct for a reserve transaction
struct Reserve {
	queue <int> roomqueue;
	int roomcounter;
	int day;
	int month;
	int year;
	int number_of_days;
	int deadline;
	string name;
};

//struct for a cancel transaction
struct Cancel {
	queue <int> roomqueue;
	int day;
	int month;
	int year;
	int roomcounter;
	int number_of_days;
	int deadline;
	string name;
};

//sruct for check transation
struct Check {
	int deadline;
	string name;
};

//struct for pay transaction
struct Pay {
	queue <int> roomqueue;
	int day;
	int month;
	int year;
	int roomcounter;
	int number_of_days;
	int deadline;
	string name;
};

//struct for a customer
struct Customer {
	int reserve_time;
	int cancel_time;
	int check_time;
	int pay_time;
	int total_computation_time;
	string name;
	queue <string> transactions;
	queue <Reserve> reservequeue;
	queue <Cancel> cancelqueue;
	queue <Pay> payqueue;
	queue <Check> checkqueue;
};



int main(int argc, char * argv[]) {
	if(argc != 2) { //ends program if incorrect arguments are used
		cout << "Usage example: PA2 input.txt" << endl;
		return -1;
	}
	string inputFile = argv[1];
	int token; //stores identifier for lex analyzer
	int numOfRooms;
	int numOfCustomers;

	//keys and IDs used for semaphore and shared memory
	key_t semkey;
	key_t shmkey;
	key_t shmkey2;
	key_t shmkey3;
	int semid;
	int shmid;
	int shmid2;
	int shmid3;

	int y; //year
	int daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	string customerNames[30];
	queue <int> deadlines;
	int customerNamesCount = 0; //number of different names

	//user choice of scheduling to use
	cout << "Enter scheduling to use: (0 = No deadlines,  1 = EDF,  2 = LLF)" << endl;
	int choice;
	cin >> choice;
	cout << endl;
	if(choice > 2 || choice < 0) {
		cout << "Incorrect input. Exiting..." << endl;
		exit(1);
	}


   //INPUT PARSING STARTS ********************
	// yylex() - gets next token from input file
	// yytext is the value of the token
	yyin = fopen(inputFile.c_str(), "r");
	yylex();
	numOfRooms = atoi(yytext);
	yylex();
	numOfCustomers = atoi(yytext);
	struct Room* rooms = new Room[numOfRooms]; //array of Room structs (one for each room)
	struct Customer* customers = new Customer[numOfCustomers]; //array of customer structs (one for each customer)
	token = yylex();
	int custCounter = -1; //customer counter
	while(token) {
		int day;
		int month;
		int year;
		int numOfDays;
		int deadline;
		string name;
		int start;
		int end;
		if(token == 5) { // "customer_x:" from input file
			custCounter++; //increment customer count
			customers[custCounter].total_computation_time = 0; //initialize customer total computation time for LLF
		}
		if(token == 1) { //reserve
			token =yylex();
			if(token == 6) { //num
				customers[custCounter].reserve_time = atoi(yytext);
			}
			else if (token == 10) { //open parenthesis
				customers[custCounter].transactions.push("reserve"); //add transaction to cust list
				token = yylex(); //get next token which is a number
				start = atoi(yytext); //store the number
				token = yylex(); //get next token
				if (token == 12) { //closed parenthesis
					yylex(); //month
					month = atoi(yytext);
					yylex(); //slash
					yylex(); //day
					day = atoi(yytext);
					yylex(); //slash
					yylex(); //year
					year = atoi(yytext);
					y = atoi(yytext);
					yylex(); //num of days
					numOfDays = atoi(yytext);
					yylex();
					name = yytext;
					yylex();
					yylex();
					deadline = atoi(yytext);
					Reserve temp;
					temp.roomcounter = 1;
					temp.name = name; temp.day = day; temp.month = month; temp.year = year; temp.number_of_days = numOfDays; temp.deadline = deadline; temp.roomqueue.push(start);
					customers[custCounter].reservequeue.push(temp);
					customers[custCounter].total_computation_time += customers[custCounter].reserve_time;
					deadlines.push(deadline);
				}
				else if(token == 11) { //dash
					yylex();
					end = atoi(yytext);
					yylex(); //closed parenthesis
					yylex(); //month
					month = atoi(yytext);
					yylex(); //slash
					yylex(); //day
					day = atoi(yytext);
					yylex(); //slash
					yylex(); //year
					year = atoi(yytext);
					y = atoi(yytext);
					yylex();
					numOfDays = atoi(yytext);
					yylex();
					name = yytext;
					yylex();
					yylex();
					deadline = atoi(yytext);
					Reserve temp;
					temp.roomcounter = 0;
					for(int i = start; i < end+1; i++) {
						temp.roomqueue.push(i);
						temp.roomcounter++;
					}

					temp.name = name; temp.day = day; temp.month = month; temp.year = year; temp.number_of_days = numOfDays; temp.deadline = deadline; temp.roomqueue.push(start);
					//cout << "reserve " << start << "-" << end <<  " " << date << " " << numOfDays << " *name* deadline " << deadline << endl;
					customers[custCounter].reservequeue.push(temp);
					customers[custCounter].total_computation_time += customers[custCounter].reserve_time;
					deadlines.push(deadline);
				}
				else if(token == 14) { //comma
					Reserve temp;
					temp.roomcounter = 0;
					temp.roomqueue.push(start);
					yylex();
					temp.roomqueue.push(atoi(yytext));
					temp.roomcounter = 2;
					token = yylex();
					while(token != 12) { //closed parenthesis
						yylex();
						temp.roomqueue.push(atoi(yytext));
						temp.roomcounter++;
						token = yylex();
					}
					yylex(); //month
					month = atoi(yytext);
					yylex(); //slash
					yylex(); //day
					day = atoi(yytext);
					yylex(); //slash
					yylex(); //year
					year = atoi(yytext);
					y = atoi(yytext);
					yylex();
					numOfDays = atoi(yytext);
					yylex();
					yylex();
					name = yytext;
					yylex();
					deadline = atoi(yytext);
					temp.name = name; temp.day = day; temp.month = month; temp.year = year; temp.number_of_days = numOfDays; temp.deadline = deadline; temp.roomqueue.push(start);
					customers[custCounter].reservequeue.push(temp);
					customers[custCounter].total_computation_time += customers[custCounter].reserve_time;
					deadlines.push(deadline);
				}
			}
		}
		if(token == 2) { //cancel
			token =yylex();
			if(token == 6) { //num
				customers[custCounter].cancel_time = atoi(yytext);
			}
			else if (token == 10) { //open parenthesis
				customers[custCounter].transactions.push("cancel"); //add transaction to cust list
				token = yylex(); //get next token which is a number
				start = atoi(yytext); //store the number
				token = yylex(); //get next token
				if (token == 12) { //closed parenthesis
					yylex(); //month
					month = atoi(yytext);
					yylex(); //slash
					yylex(); //day
					day = atoi(yytext);
					yylex(); //slash
					yylex(); //year
					year = atoi(yytext);
					yylex();
					numOfDays = atoi(yytext);
					yylex();
					name = yytext;
					yylex();
					yylex();
					deadline = atoi(yytext);
					Cancel temp;
					temp.roomcounter = 1;
					temp.name = name; temp.day = day; temp.month = month; temp.year = year; temp.number_of_days = numOfDays; temp.deadline = deadline; temp.roomqueue.push(start);
					customers[custCounter].cancelqueue.push(temp);
					customers[custCounter].total_computation_time += customers[custCounter].cancel_time;
					deadlines.push(deadline);
				}
				else if(token == 11) { //dash
					yylex();
					end = atoi(yytext);
					yylex(); //closed parenthesis
					yylex(); //month
					month = atoi(yytext);
					yylex(); //slash
					yylex(); //day
					day = atoi(yytext);
					yylex(); //slash
					yylex(); //year
					year = atoi(yytext);
					yylex();
					numOfDays = atoi(yytext);
					yylex();
					name = yytext;
					yylex();
					yylex();
					deadline = atoi(yytext);
					Cancel temp;
					temp.roomcounter = 0;
					for(int i = start; i < end+1; i++) {
						temp.roomqueue.push(i);
						temp.roomcounter++;
					}
					temp.name = name; temp.day = day; temp.month = month; temp.year = year; temp.number_of_days = numOfDays; temp.deadline = deadline; temp.roomqueue.push(start);
					customers[custCounter].cancelqueue.push(temp);
					customers[custCounter].total_computation_time += customers[custCounter].cancel_time;
					deadlines.push(deadline);
				}
				else if(token == 14) { //comma
					Cancel temp;
					temp.roomqueue.push(start);
					yylex();
					temp.roomqueue.push(atoi(yytext));
					temp.roomcounter = 2;
					token = yylex();
					while(token != 12) { //closed parenthesis
						yylex();
						temp.roomqueue.push(atoi(yytext));
						temp.roomcounter++;
						token = yylex();
					}
					yylex(); //month
					month = atoi(yytext);
					yylex(); //slash
					yylex(); //day
					day = atoi(yytext);
					yylex(); //slash
					yylex(); //year
					year = atoi(yytext);
					yylex();
					numOfDays = atoi(yytext);
					yylex();
					name = yytext;
					yylex();
					yylex();
					deadline = atoi(yytext);
					temp.name = name; temp.day = day; temp.month = month; temp.year = year; temp.number_of_days = numOfDays; temp.deadline = deadline; temp.roomqueue.push(start);
					customers[custCounter].cancelqueue.push(temp);
					customers[custCounter].total_computation_time += customers[custCounter].cancel_time;
					deadlines.push(deadline);
				}
			}
		}
		if(token == 3) { //check
			token = yylex();
			if(token == 6) { //number
				customers[custCounter].check_time = atoi(yytext);
			}
			if(token == 9) { //name
				name = yytext;
				customers[custCounter].transactions.push("check"); //add transaction to cust list
				yylex(); //deadline
				yylex();
				deadline = atoi(yytext);
				Check temp;
				temp.name = name;
				temp.deadline = deadline;
				customers[custCounter].checkqueue.push(temp);
				customers[custCounter].total_computation_time += customers[custCounter].check_time;
				deadlines.push(deadline);
			}
		}
		if(token == 4) { //pay
			token =yylex();
			if(token == 6) { //num
				customers[custCounter].pay_time = atoi(yytext);
			}
			else if (token == 10) { //open parenthesis
				customers[custCounter].transactions.push("pay"); //add transaction to cust list
				token = yylex(); //get next token which is a number
				start = atoi(yytext); //store the number
				token = yylex(); //get next token
				if (token == 12) { //closed parenthesis
					yylex(); //month
					month = atoi(yytext);
					yylex(); //slash
					yylex(); //day
					day = atoi(yytext);
					yylex(); //slash
					yylex(); //year
					year = atoi(yytext);
					yylex();
					numOfDays = atoi(yytext);
					yylex();
					name = yytext;
					yylex();
					yylex();
					deadline = atoi(yytext);
					Pay temp;
					temp.roomcounter = 1;
					temp.name = name; temp.day = day; temp.month = month; temp.year = year; temp.number_of_days = numOfDays; temp.deadline = deadline; temp.roomqueue.push(start);
					customers[custCounter].payqueue.push(temp);
					customers[custCounter].total_computation_time += customers[custCounter].pay_time;
					deadlines.push(deadline);
				}
				else if(token == 11) { //dash
					yylex();
					end = atoi(yytext);
					yylex(); //closed parenthesis
					yylex(); //month
					month = atoi(yytext);
					yylex(); //slash
					yylex(); //day
					day = atoi(yytext);
					yylex(); //slash
					yylex(); //year
					year = atoi(yytext);
					yylex();
					numOfDays = atoi(yytext);
					yylex();
					name = yytext;
					yylex();
					yylex();
					deadline = atoi(yytext);
					Pay temp;
					temp.roomcounter = 0;
					for(int i = start; i < end+1; i++) {
						temp.roomqueue.push(i);
						temp.roomcounter++;
					}
					temp.name = name; temp.day = day; temp.month = month; temp.year = year; temp.number_of_days = numOfDays; temp.deadline = deadline; temp.roomqueue.push(start);
					customers[custCounter].payqueue.push(temp);
					customers[custCounter].total_computation_time += customers[custCounter].pay_time;
					deadlines.push(deadline);
				}
				else if(token == 14) { //comma
					Pay temp;
					temp.roomqueue.push(start);
					yylex();
					temp.roomqueue.push(atoi(yytext));
					temp.roomcounter = 2;
					token = yylex();
					while(token != 12) { //closed parenthesis
						yylex();
						temp.roomqueue.push(atoi(yytext));
						temp.roomcounter++;
						token = yylex();
					}
					yylex(); //month
					month = atoi(yytext);
					yylex(); //slash
					yylex(); //day
					day = atoi(yytext);
					yylex(); //slash
					yylex(); //year
					year = atoi(yytext);
					yylex();
					numOfDays = atoi(yytext);
					yylex();
					name = yytext;
					yylex();
					yylex();
					deadline = atoi(yytext);
					temp.name = name; temp.day = day; temp.month = month; temp.year = year; temp.number_of_days = numOfDays; temp.deadline = deadline; temp.roomqueue.push(start);
					customers[custCounter].payqueue.push(temp);
					customers[custCounter].total_computation_time += customers[custCounter].pay_time;
					deadlines.push(deadline);
				}
			}
		}

		//adds name to Customer name array if its a new name
		bool found = false;
		for(int x = 0; x < 30; x++) {
			if(name == customerNames[x]) {
				found = true;
			}
		}
		if(found == false) {
			customerNames[customerNamesCount] = name;
			customerNamesCount++;
		}

		//read next element from text file and goes back to beginning of while loop
		token = yylex();
	}
	// INPUT PARSING ENDS ********************


	//transfers deadlines of every transaction into array, and sorts it to get in EDF order
	int numOfDeadlines = deadlines.size();
	int *deadlines_array = new int[numOfDeadlines];
	for(int i = 0; i < numOfDeadlines; i++) {
		deadlines_array[i] = deadlines.front();
		deadlines.pop();
	}
	sort(deadlines_array, deadlines_array+sizeof(deadlines_array));



	//initializes rooms 3d array
	for(int i = 0; i < numOfRooms; i++) {
		rooms[i].room_counter = 0;
		for(int j = 0; j < 12; j++) {
			for(int x = 0; x < 31; x++) {
				rooms[i].date_booked[j][x][0] = -1;
				rooms[i].date_booked[j][x][1] = -1;
				rooms[i].date_booked[j][x][2] = -1;
			}
		}
	}

	//Creates and initializes semaphore
	if((semid = semget(semkey, 0, 0)) == -1) {
		if((semid = semget(semkey, 1, IPC_CREAT | 0666)) != -1) {
			struct sembuf sbuf;
			sbuf.sem_num = 0; //sem number to initialize
			sbuf.sem_op = 1; //initial sem value
			sbuf.sem_flg = 0;
			if(semop(semid, &sbuf, 1) == -1) { //operation to initialize
				perror("IPC Error: semop");
				exit(1); //exit if error initializing sem
			}
		}
		else {
			perror("Error creating semaphore");
			exit(1);
		}
	}

	//creates shared memory space
	int size = sizeof(Room)*numOfRooms;
	if((shmid = shmget(shmkey, size, IPC_CREAT | 0666)) < 0) {
		perror("Failed to allocate shared mem");
		exit(1);
	}

	//creates shared memory space
	int size2 = sizeof(int*);
	if((shmid2 = shmget(shmkey2, size2, IPC_CREAT | 0666)) < 0) {
		perror("Failed to allocate shared mem");
		exit(1);
	}

	//attaches shared memory to process
	Room* shared_memory;
	shared_memory = (Room*)shmat(shmid, NULL, 0);
	if(!shared_memory) {
		perror("Error attaching");
		exit(0);
	}

	//attaches shared memory to process
	int* shared_value;
	shared_value = (int*)shmat(shmid, NULL, 0);
	if(!shared_value) {
		perror("Error attaching");
		exit(0);
	}
	*shared_value = 0; //counter used in EDF


	struct Room *PTR = rooms; //temp pointer to rooms array
	copy(&rooms[0], &rooms[numOfRooms], shared_memory); //copy rooms array into sh memory
	rooms = (struct Room*) shared_memory;
	delete [] PTR; //deletes the memory location where rooms was stored before being in shared memory


	int pid[numOfCustomers]; //for fork later


	//******************CHOICE 0 (NO DEADLINES)**************************
	if(choice == 0) {
	//loop that will create the correct number of child processes (one for each customer)
	for(int i = 0; i < numOfCustomers; i++) {
		int cust = i+1; //current customer
		pid[i] = fork(); //fork and create child process
		if(pid[i] < 0) { //error forking, end program
			perror("Bad fork");
		}
		else if(pid[i] == 0) { //is child i (Each customer is a child process)
			while(!customers[i].transactions.empty()) { //runs until customer finishes all transactions
				string op = customers[i].transactions.front(); //get first operation for this customer
				if(op == "reserve") {
					if(sem_wait(semid, 0)) //issues a wait on the semaphore
						exit(1);
					Reserve tempq = customers[i].reservequeue.front(); //gets the reserve transaction
					for(int j = 0; j < tempq.roomcounter; j++) { //runs for every room in this transaction
						int temproom = tempq.roomqueue.front();
						int startday = tempq.day;
						int startyear = tempq.year;
						string name = tempq.name;
						int nameArraySpot;
						for(int x = 0; x < customerNamesCount; x++) {
							if(name == customerNames[x])
								nameArraySpot = x;
						}
						int startmonth = tempq.month;
						int days = tempq.number_of_days;
						for(int i = 0; i < days; i++) {
							//valid checks if the room is booked for that day (-1 = not booked)
							int valid = shared_memory[temproom-1].date_booked[startmonth-1][startday-1][0];
							if((startday < daysInMonth[startmonth-1]) && (valid == -1)) { //reserve on that day
								cout << "Customer " << cust << " (" << name <<  ") Reserved room " << temproom << " on " << startmonth << "/" << startday << "/" << y << endl;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][0] = cust;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][2] = nameArraySpot;
								startday++;
							}
							else if((startday == daysInMonth[startmonth-1]) && (valid == -1)) { //reserve on that day and move to next month
								cout << "Customer " << cust << " (" << name <<  ") Reserved room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << endl;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][0] = cust;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][2] = nameArraySpot;
								startday = 1;
								startmonth++;
							}
							else { //already reserved
								cout << "Customer " << cust << " (" << name <<  ") Could not reserve room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << " because it's already taken" << endl;
								if(startday < daysInMonth[startmonth-1]) {
									startday++;
								}
								else if(startday == daysInMonth[startmonth-1]) {
									startday = 1;
									startmonth++;
								}
							}
						}
						cout << endl;
						tempq.roomqueue.pop();
					}
					customers[i].reservequeue.pop(); //done with current transaction
					usleep(customers[i].reserve_time * 1000); //sleep for x ms
					if(sem_signal(semid, 0)) //semaphore signal
						exit(1);
				}
				else if(op == "cancel") {
					if(sem_wait(semid, 0))
						exit(1);
					Cancel tempq = customers[i].cancelqueue.front();
					for(int j = 0; j < tempq.roomcounter; j++) {
						int temproom = tempq.roomqueue.front();
						int startday = tempq.day;
						int startmonth = tempq.month;
						string name = tempq.name;
						int nameArraySpot;
						for(int x = 0; x < customerNamesCount; x++) {
							if(name == customerNames[x])
								nameArraySpot = x;
						}
						int days = tempq.number_of_days;
						for(int i = 0; i < days; i++) {
							int valid = shared_memory[temproom-1].date_booked[startmonth-1][startday-1][0];
							int paid = shared_memory[temproom-1].date_booked[startmonth-1][startday-1][1];
							if((startday < daysInMonth[startmonth-1]) && (valid == cust) && (paid == -1)) {
								cout << "Customer " << cust << " (" << name <<  ") Canceled room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << endl;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][0] = -1;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][2] = -1;
								startday++;
							}
							else if((startday == daysInMonth[startmonth-1]) && (valid == cust) && (paid == -1)) {
								cout << "Customer " << cust << " (" << name <<  ") Canceled room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << endl;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][0] = -1;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][2] = -1;
								startday = 1;
								startmonth++;
							}
							else if((startday == daysInMonth[startmonth-1]) && (valid == cust) && (paid == 1)) {
								cout << "Customer " << cust << " (" << name <<  ") Could not cancel room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << " because it's already paid" << endl;
								if(startday < daysInMonth[startmonth-1]) {
									startday++;
								}
								else if(startday == daysInMonth[startmonth-1]) {
									startday = 1;
									startmonth++;
								}
							}
							else {
								cout << "Customer " << cust << " (" << name <<  ") Could not cancel room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << endl;
								if(startday < daysInMonth[startmonth-1]) {
									startday++;
								}
								else if(startday == daysInMonth[startmonth-1]) {
									startday = 1;
									startmonth++;
								}
							}
						}
						cout << endl;
						tempq.roomqueue.pop();
					}
					customers[i].cancelqueue.pop();
					usleep(customers[i].cancel_time * 1000);
					if(sem_signal(semid, 0))
						exit(1);
				}
				else if(op == "check") {
					if(sem_wait(semid, 0))
						exit(1);
					Check tempq = customers[i].checkqueue.front();
					string name = tempq.name;
					cout << "CUSTOMER " << i+1 << " RESERVATIONS" << endl;
					cout << "------------------------" << endl;
					cout << setw(12) << left << "Name:" << setw(12) << left << "Room:" << setw(12) << left << "Date:" << setw(12) << left << "	Paid:" << endl;
					for(int r = 0; r < numOfRooms; r++) {
						for(int j = 0; j < 12; j++) {
							for(int x = 0; x<31; x++) {
								if(shared_memory[r].date_booked[j][x][0] == cust) {
									if(shared_memory[r].date_booked[j][x][1] == 1) {
										int nameCounter = shared_memory[r].date_booked[j][x][2];
										cout << setw(12) << left << customerNames[nameCounter] << setw(12) << left << r+1  << j+1 << "/" << x+1 << "/" << y  << setw(18) << left << "	yes" << endl;
									}
									else {
										int nameCounter = shared_memory[r].date_booked[j][x][2];
										cout << setw(12) << left << customerNames[nameCounter] << setw(12) << left << r+1  << j+1 << "/" << x+1 << "/" << y  << setw(18) << left << "	no" << endl;
									}
								}
							}
						}
					}
					cout << endl;
					customers[i].checkqueue.pop();
					usleep(customers[i].check_time * 1000);
					if(sem_signal(semid, 0))
						exit(1);
				}
				else if(op == "pay") {
					if(sem_wait(semid, 0))
						exit(1);
					Pay tempq = customers[i].payqueue.front();
					for(int j = 0; j < tempq.roomcounter; j++) {
						int temproom = tempq.roomqueue.front();
						int startday = tempq.day;
						string name = tempq.name;
						int nameArraySpot;
						for(int x = 0; x < customerNamesCount; x++) {
							if(name == customerNames[x])
								nameArraySpot = x;
						}
						int startmonth = tempq.month;
						int days = tempq.number_of_days;
						for(int i = 0; i < days; i++) {
							int valid = shared_memory[temproom-1].date_booked[startmonth-1][startday-1][0];
							if((startday < daysInMonth[startmonth-1]) && (valid == cust)) {
								cout << "Customer " << cust << " (" << name <<  ") Paid for room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << endl;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][1] = 1;
								startday++;
							}
							else if((startday == daysInMonth[startmonth-1]) && (valid == cust)) {
								cout << "Customer " << cust << " (" << name <<  ") Paid for room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << endl;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][1] = 1;
								startday = 1;
								startmonth++;
							}
							else {
								cout << "Customer " << cust << " (" << name <<  ") Could not pay for room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << endl;
								if(startday < daysInMonth[startmonth-1]) {
									startday++;
								}
								else if(startday == daysInMonth[startmonth-1]) {
									startday = 1;
									startmonth++;
								}
							}
						}
						cout << endl;
						tempq.roomqueue.pop();
					}
					customers[i].payqueue.pop();
					usleep(customers[i].pay_time * 1000);
					if(sem_signal(semid, 0))
						exit(1);
				}
				customers[i].transactions.pop();
			}


			exit(0);
		}
		else { //is parent

		}
	}
	}
	//******************CHOICE 1 (EDF)**************************
	if(choice == 1) {
	//loop that will create the correct number of child processes (one for each customer)
	for(int i = 0; i < numOfCustomers; i++) {
		int cust = i+1; //current customer
		*shared_value = 0;
		pid[i] = fork(); //fork and create child process
		struct timespec created, current;
		//create time reference at process start time
		clock_gettime(CLOCK_MONOTONIC, &created);
		double elapsed; //variable that will be used to calculate elapsed time at various moments
		if(pid[i] < 0) { //error forking, end program
			perror("Bad fork");
		}
		else if(pid[i] == 0) { //is child i (Each customer is a child process)
			while(numOfDeadlines > *shared_value) { //runs until customer finishes all transactions
				int currentDeadline = deadlines_array[*shared_value];
				int search = -1;
				string op = "";
				while((op == "") && (numOfDeadlines != *shared_value)) { //stays in loop until its this customer's turn
					//stays in while loop until this process's next transaction has the earliest deadline
					int refresh = *shared_value;
					currentDeadline = deadlines_array[refresh];
					for(int k = 0; k < customers[i].reservequeue.size(); k++) {
						Reserve tempr = customers[i].reservequeue.front();
						if(tempr.deadline == currentDeadline) {
							search = tempr.deadline;
							op = "reserve";
						}
						else {
							customers[i].reservequeue.pop();
							customers[i].reservequeue.push(tempr);
						}
					}
					for(int k = 0; k < customers[i].cancelqueue.size(); k++) {
						Cancel tempr = customers[i].cancelqueue.front();
						if(tempr.deadline == currentDeadline) {
							search = tempr.deadline;
							op = "cancel";
						}
						else {
							customers[i].cancelqueue.pop();
							customers[i].cancelqueue.push(tempr);
						}
					}
					for(int k = 0; k < customers[i].payqueue.size(); k++) {
						Pay tempr = customers[i].payqueue.front();
						if(tempr.deadline == currentDeadline) {
							search = tempr.deadline;
							op = "pay";
						}
						else {
							customers[i].payqueue.pop();
							customers[i].payqueue.push(tempr);
						}
					}
					for(int k = 0; k < customers[i].checkqueue.size(); k++) {
						Check tempr = customers[i].checkqueue.front();
						if(tempr.deadline == currentDeadline) {
							search = tempr.deadline;
							op = "check";
						}
						else {
							customers[i].checkqueue.pop();
							customers[i].checkqueue.push(tempr);
						}
					}
				}
				if(op == "reserve") {
					if(sem_wait(semid, 0))
						exit(1);
					Reserve tempq = customers[i].reservequeue.front();
					for(int j = 0; j < tempq.roomcounter; j++) {
						int temproom = tempq.roomqueue.front();
						int startday = tempq.day;
						int startyear = tempq.year;
						string name = tempq.name;
						int nameArraySpot;
						for(int x = 0; x < customerNamesCount; x++) {
							if(name == customerNames[x])
								nameArraySpot = x;
						}
						int startmonth = tempq.month;
						int days = tempq.number_of_days;
						for(int i = 0; i < days; i++) {
							int valid = shared_memory[temproom-1].date_booked[startmonth-1][startday-1][0];
							if((startday < daysInMonth[startmonth-1]) && (valid == -1)) {
								cout << "Customer " << cust << " (" << name <<  ") Reserved room " << temproom << " on " << startmonth << "/" << startday << "/" << y << endl;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][0] = cust;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][2] = nameArraySpot;
								cout << shared_memory[temproom-1].date_booked[startmonth-1][startday-1][2] << endl;
								startday++;
							}
							else if((startday == daysInMonth[startmonth-1]) && (valid == -1)) {
								cout << "Customer " << cust << " (" << name <<  ") Reserved room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << endl;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][0] = cust;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][2] = nameArraySpot;
								cout << shared_memory[temproom-1].date_booked[startmonth-1][startday-1][2] << endl;
								startday = 1;
								startmonth++;
							}
							else {
								cout << "Customer " << cust << " (" << name <<  ") Could not reserve room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << " because it's already taken" << endl;
								if(startday < daysInMonth[startmonth-1]) {
									startday++;
								}
								else if(startday == daysInMonth[startmonth-1]) {
									startday = 1;
									startmonth++;
								}
							}
						}
						tempq.roomqueue.pop();
					}
					customers[i].reservequeue.pop();
					usleep(customers[i].reserve_time * 1000);
					clock_gettime(CLOCK_MONOTONIC, &current);
					elapsed = (current.tv_sec - created.tv_sec);
					elapsed += (current.tv_nsec - created.tv_nsec) / 1000000.0;
					cout << "Elapsed: " << elapsed << "ms" << endl;
					if(elapsed < tempq.deadline) {
						cout << "Deadline of " << tempq.deadline << "ms met." << endl;
					}
					else
						cout << "Deadline of " << tempq.deadline << "ms missed by " << elapsed - tempq.deadline << "ms." << endl;
					cout << endl;
					if(sem_signal(semid, 0))
						exit(1);
				}
				else if(op == "cancel") {
					if(sem_wait(semid, 0))
						exit(1);
					Cancel tempq = customers[i].cancelqueue.front();
					for(int j = 0; j < tempq.roomcounter; j++) {
						int temproom = tempq.roomqueue.front();
						int startday = tempq.day;
						int startmonth = tempq.month;
						string name = tempq.name;
						int nameArraySpot;
						for(int x = 0; x < customerNamesCount; x++) {
							if(name == customerNames[x])
								nameArraySpot = x;
						}
						int days = tempq.number_of_days;
						for(int i = 0; i < days; i++) {
							int valid = shared_memory[temproom-1].date_booked[startmonth-1][startday-1][0];
							int paid = shared_memory[temproom-1].date_booked[startmonth-1][startday-1][1];
							if((startday < daysInMonth[startmonth-1]) && (valid == cust) && (paid == -1)) {
								cout << "Customer " << cust << " (" << name <<  ") Canceled room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << endl;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][0] = -1;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][2] = -1;
								startday++;
							}
							else if((startday == daysInMonth[startmonth-1]) && (valid == cust) && (paid == -1)) {
								cout << "Customer " << cust << " (" << name <<  ") Canceled room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << endl;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][0] = -1;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][2] = -1;
								startday = 1;
								startmonth++;
							}
							else if((startday == daysInMonth[startmonth-1]) && (valid == cust) && (paid == 1)) {
								cout << "Customer " << cust << " (" << name <<  ") Could not cancel room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << " because it's already paid" << endl;
								if(startday < daysInMonth[startmonth-1]) {
									startday++;
								}
								else if(startday == daysInMonth[startmonth-1]) {
									startday = 1;
									startmonth++;
								}
							}
							else {
								cout << "Customer " << cust << " (" << name <<  ") Could not cancel room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << endl;
								if(startday < daysInMonth[startmonth-1]) {
									startday++;
								}
								else if(startday == daysInMonth[startmonth-1]) {
									startday = 1;
									startmonth++;
								}
							}
						}
						tempq.roomqueue.pop();
					}
					customers[i].cancelqueue.pop();
					usleep(customers[i].cancel_time * 1000);
					clock_gettime(CLOCK_MONOTONIC, &current);
					elapsed = (current.tv_sec - created.tv_sec);
					elapsed += (current.tv_nsec - created.tv_nsec) / 1000000.0;
					cout << "Elapsed: " << elapsed << "ms" << endl;
					if(elapsed < tempq.deadline) {
						cout << "Deadline of " << tempq.deadline << "ms met." << endl;
					}
					else
						cout << "Deadline of " << tempq.deadline << "ms missed by " << elapsed - tempq.deadline << "ms." << endl;
					cout << endl;
					if(sem_signal(semid, 0))
						exit(1);


				}
				else if(op == "check") {
					if(sem_wait(semid, 0))
						exit(1);
					Check tempq = customers[i].checkqueue.front();
					string name = tempq.name;
					cout << "CUSTOMER " << i+1 << " RESERVATIONS" << endl;
					cout << "------------------------" << endl;
					cout << setw(12) << left << "Name:" << setw(12) << left << "Room:" << setw(12) << left << "Date:" << setw(12) << left << "	Paid:" << endl;
					for(int r = 0; r < numOfRooms; r++) {
						for(int j = 0; j < 12; j++) {
							for(int x = 0; x<31; x++) {
								if(shared_memory[r].date_booked[j][x][0] == cust) {
									if(shared_memory[r].date_booked[j][x][1] == 1) {
										int nameCounter = shared_memory[r].date_booked[j][x][2];
										cout << setw(12) << left << customerNames[nameCounter] << setw(12) << left << r+1  << j+1 << "/" << x+1 << "/" << y  << setw(18) << left << "	yes" << endl;
									}
									else {
										int nameCounter = shared_memory[r].date_booked[j][x][2];
										cout << setw(12) << left << customerNames[nameCounter] << setw(12) << left << r+1  << j+1 << "/" << x+1 << "/" << y  << setw(18) << left << "	no" << endl;
									}
								}
							}
						}
					}
					customers[i].checkqueue.pop();
					usleep(customers[i].check_time * 1000);
					clock_gettime(CLOCK_MONOTONIC, &current);
					elapsed = (current.tv_sec - created.tv_sec);
					elapsed += (current.tv_nsec - created.tv_nsec) / 1000000.0;
					cout << "Elapsed: " << elapsed << "ms" << endl;
					if(elapsed < tempq.deadline) {
						cout << "Deadline of " << tempq.deadline << "ms met." << endl;
					}
					else
						cout << "Deadline of " << tempq.deadline << "ms missed by " << elapsed - tempq.deadline << "ms." << endl;
					cout << endl;
					if(sem_signal(semid, 0))
						exit(1);
				}
				else if(op == "pay") {
					if(sem_wait(semid, 0))
						exit(1);
					Pay tempq = customers[i].payqueue.front();
					for(int j = 0; j < tempq.roomcounter; j++) {
						int temproom = tempq.roomqueue.front();
						int startday = tempq.day;
						string name = tempq.name;
						int nameArraySpot;
						for(int x = 0; x < customerNamesCount; x++) {
							if(name == customerNames[x])
								nameArraySpot = x;
						}
						int startmonth = tempq.month;
						int days = tempq.number_of_days;
						for(int i = 0; i < days; i++) {
							int valid = shared_memory[temproom-1].date_booked[startmonth-1][startday-1][0];
							if((startday < daysInMonth[startmonth-1]) && (valid == cust)) {
								cout << "Customer " << cust << " (" << name <<  ") Paid for room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << endl;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][1] = 1;
								startday++;
							}
							else if((startday == daysInMonth[startmonth-1]) && (valid == cust)) {
								cout << "Customer " << cust << " (" << name <<  ") Paid for room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << endl;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][1] = 1;
								startday = 1;
								startmonth++;
							}
							else {
								cout << "Customer " << cust << " (" << name <<  ") Could not pay for room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << endl;
								if(startday < daysInMonth[startmonth-1]) {
									startday++;
								}
								else if(startday == daysInMonth[startmonth-1]) {
									startday = 1;
									startmonth++;
								}
							}
						}
						tempq.roomqueue.pop();
					}
					customers[i].payqueue.pop();
					usleep(customers[i].pay_time * 1000);
					clock_gettime(CLOCK_MONOTONIC, &current);
					elapsed = (current.tv_sec - created.tv_sec);
					elapsed += (current.tv_nsec - created.tv_nsec) / 1000000.0;
					cout << "Elapsed: " << elapsed << "ms" << endl;
					if(elapsed < tempq.deadline) {
						cout << "Deadline of " << tempq.deadline << "ms met." << endl;
					}
					else
						cout << "Deadline of " << tempq.deadline << "ms missed by " << elapsed - tempq.deadline << "ms." << endl;
					cout << endl;
					if(sem_signal(semid, 0))
						exit(1);
				}

				*shared_value+= 1;

			}


			exit(0);
		}
		else { //is parent

		}
	}
	}

	//******************CHOICE 2 (LLF)**************************
	if(choice == 2) {
		double* laxity = new double[numOfCustomers]; //for LLF - stores current laxity of each process
		//creates shared memory space
		int size3 = sizeof(laxity);
		if((shmid3 = shmget(shmkey3, size3, IPC_CREAT | 0666)) < 0) {
			perror("Failed to allocate shared mem");
			exit(1);
		}

		//attaches shared memory to process
		laxity = (double*)shmat(shmid3, NULL, 0);
		if(!laxity) {
			perror("Error attaching");
			exit(0);
		}
	//loop that will create the correct number of child processes (one for each customer)
	for(int i = 0; i < numOfCustomers; i++) {
		int cust = i+1; //current customer
		pid[i] = fork(); //fork and create child process
		struct timespec created, current;
		//create time reference at process start time
		clock_gettime(CLOCK_MONOTONIC, &created);
		double elapsed; //variable that will be used to calculate elapsed time at various moments

		if(pid[i] < 0) { //error forking, end program
			perror("Bad fork");
		}
		else if(pid[i] == 0) { //is child i (Each customer is a child process)
			while(!customers[i].transactions.empty()) { //runs until customer finishes all transactions
				string op = customers[i].transactions.front(); //gets next transaction to run for this process
				//Calculats laxity of process to determine which process to run next
				if(op == "reserve") {
					Reserve tempq = customers[i].reservequeue.front();
					int deadline = tempq.deadline;
					clock_gettime(CLOCK_MONOTONIC, &current);
					elapsed = (current.tv_sec - created.tv_sec);
					elapsed += (current.tv_nsec - created.tv_nsec) / 1000000.0;
					laxity[i] = (deadline - elapsed) - customers[i].total_computation_time;
				}
				else if(op == "cancel") {
					Cancel tempq = customers[i].cancelqueue.front();
					int deadline = tempq.deadline;
					clock_gettime(CLOCK_MONOTONIC, &current);
					elapsed = (current.tv_sec - created.tv_sec);
					elapsed += (current.tv_nsec - created.tv_nsec) / 1000000.0;
					laxity[i] = (deadline - elapsed) - customers[i].total_computation_time;
				}
				else if(op == "check") {
					Check tempq = customers[i].checkqueue.front();
					int deadline = tempq.deadline;
					clock_gettime(CLOCK_MONOTONIC, &current);
					elapsed = (current.tv_sec - created.tv_sec);
					elapsed += (current.tv_nsec - created.tv_nsec) / 1000000.0;
					laxity[i] = (deadline - elapsed) - customers[i].total_computation_time;
				}
				else if(op == "pay") {
					Pay tempq = customers[i].payqueue.front();
					int deadline = tempq.deadline;
					clock_gettime(CLOCK_MONOTONIC, &current);
					elapsed = (current.tv_sec - created.tv_sec);
					elapsed += (current.tv_nsec - created.tv_nsec) / 1000000.0;
					laxity[i] = (deadline - elapsed) - customers[i].total_computation_time;
				}

				bool myTurn = false; //true when its this process's turn to run
				while(myTurn == false) {
					double lowest = laxity[0];
					for(int x = 0; x < numOfCustomers; x++) { //finds lowest laxity from every process
						if(laxity[x] < lowest) {
							lowest = laxity[x];
						}
					}
					if(laxity[i] == lowest) //if current process has lowest laxity, it will run
						myTurn = true;
				}
				if(op == "reserve") {
					if(sem_wait(semid, 0))
						exit(1);
					Reserve tempq = customers[i].reservequeue.front();
					for(int j = 0; j < tempq.roomcounter; j++) {
						int temproom = tempq.roomqueue.front();
						int startday = tempq.day;
						int startyear = tempq.year;
						string name = tempq.name;
						int nameArraySpot;
						for(int x = 0; x < customerNamesCount; x++) {
							if(name == customerNames[x])
								nameArraySpot = x;
						}
						int startmonth = tempq.month;
						int days = tempq.number_of_days;
						for(int i = 0; i < days; i++) {
							int valid = shared_memory[temproom-1].date_booked[startmonth-1][startday-1][0];
							if((startday < daysInMonth[startmonth-1]) && (valid == -1)) {
								cout << "Customer " << cust << " (" << name <<  ") Reserved room " << temproom << " on " << startmonth << "/" << startday << "/" << y << endl;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][0] = cust;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][2] = nameArraySpot;
								startday++;
							}
							else if((startday == daysInMonth[startmonth-1]) && (valid == -1)) {
								cout << "Customer " << cust << " (" << name <<  ") Reserved room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << endl;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][0] = cust;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][2] = nameArraySpot;
								startday = 1;
								startmonth++;
							}
							else {
								cout << "Customer " << cust << " (" << name <<  ") Could not reserve room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << " because it's already taken" << endl;
								if(startday < daysInMonth[startmonth-1]) {
									startday++;
								}
								else if(startday == daysInMonth[startmonth-1]) {
									startday = 1;
									startmonth++;
								}
							}
						}
						tempq.roomqueue.pop();
					}
					customers[i].reservequeue.pop();
					usleep(customers[i].reserve_time * 1000);
					clock_gettime(CLOCK_MONOTONIC, &current);
					elapsed = (current.tv_sec - created.tv_sec);
					elapsed += (current.tv_nsec - created.tv_nsec) / 1000000.0;
					customers[i].total_computation_time = customers[i].total_computation_time - customers[i].reserve_time;
					cout << "Elapsed: " << elapsed << "ms" << endl;
					if(elapsed < tempq.deadline) {
						cout << "Deadline of " << tempq.deadline << "ms met." << endl;
					}
					else
						cout << "Deadline of " << tempq.deadline << "ms missed by " << elapsed - tempq.deadline << "ms." << endl;
					cout << endl;
					if(sem_signal(semid, 0))
						exit(1);
				}
				else if(op == "cancel") {
					if(sem_wait(semid, 0))
						exit(1);
					Cancel tempq = customers[i].cancelqueue.front();
					for(int j = 0; j < tempq.roomcounter; j++) {
						int temproom = tempq.roomqueue.front();
						int startday = tempq.day;
						int startmonth = tempq.month;
						string name = tempq.name;
						int nameArraySpot;
						for(int x = 0; x < customerNamesCount; x++) {
							if(name == customerNames[x])
								nameArraySpot = x;
						}
						int days = tempq.number_of_days;
						for(int i = 0; i < days; i++) {
							int valid = shared_memory[temproom-1].date_booked[startmonth-1][startday-1][0];
							int paid = shared_memory[temproom-1].date_booked[startmonth-1][startday-1][1];
							if((startday < daysInMonth[startmonth-1]) && (valid == cust) && (paid == -1)) {
								cout << "Customer " << cust << " (" << name <<  ") Canceled room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << endl;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][0] = -1;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][2] = -1;
								startday++;
							}
							else if((startday == daysInMonth[startmonth-1]) && (valid == cust) && (paid == -1)) {
								cout << "Customer " << cust << " (" << name <<  ") Canceled room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << endl;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][0] = -1;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][2] = -1;
								startday = 1;
								startmonth++;
							}
							else if((startday == daysInMonth[startmonth-1]) && (valid == cust) && (paid == 1)) {
								cout << "Customer " << cust << " (" << name <<  ") Could not cancel room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << " because it's already paid" << endl;
								if(startday < daysInMonth[startmonth-1]) {
									startday++;
								}
								else if(startday == daysInMonth[startmonth-1]) {
									startday = 1;
									startmonth++;
								}
							}
							else {
								cout << "Customer " << cust << " (" << name <<  ") Could not cancel room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << endl;
								if(startday < daysInMonth[startmonth-1]) {
									startday++;
								}
								else if(startday == daysInMonth[startmonth-1]) {
									startday = 1;
									startmonth++;
								}
							}
						}
						tempq.roomqueue.pop();
					}
					customers[i].cancelqueue.pop();
					usleep(customers[i].cancel_time * 1000);
					clock_gettime(CLOCK_MONOTONIC, &current);
					elapsed = (current.tv_sec - created.tv_sec);
					elapsed += (current.tv_nsec - created.tv_nsec) / 1000000.0;
					customers[i].total_computation_time = customers[i].total_computation_time - customers[i].cancel_time;
					cout << "Elapsed: " << elapsed << "ms" << endl;
					if(elapsed < tempq.deadline) {
						cout << "Deadline of " << tempq.deadline << "ms met." << endl;
					}
					else
						cout << "Deadline of " << tempq.deadline << "ms missed by " << elapsed - tempq.deadline << "ms." << endl;
					cout << endl;
					if(sem_signal(semid, 0))
						exit(1);
				}
				else if(op == "check") {
					if(sem_wait(semid, 0))
						exit(1);
					Check tempq = customers[i].checkqueue.front();
					string name = tempq.name;
					cout << "CUSTOMER " << i+1 << " RESERVATIONS" << endl;
					cout << "------------------------" << endl;
					cout << setw(12) << left << "Name:" << setw(12) << left << "Room:" << setw(12) << left << "Date:" << setw(12) << left << "	Paid:" << endl;
					for(int r = 0; r < numOfRooms; r++) {
						for(int j = 0; j < 12; j++) {
							for(int x = 0; x<31; x++) {
								if(shared_memory[r].date_booked[j][x][0] == cust) {
									if(shared_memory[r].date_booked[j][x][1] == 1) {
										int nameCounter = shared_memory[r].date_booked[j][x][2];
										cout << setw(12) << left << customerNames[nameCounter] << setw(12) << left << r+1  << j+1 << "/" << x+1 << "/" << y  << setw(18) << left << "	yes" << endl;
									}
									else {
										int nameCounter = shared_memory[r].date_booked[j][x][2];
										cout << setw(12) << left << customerNames[nameCounter] << setw(12) << left << r+1  << j+1 << "/" << x+1 << "/" << y  << setw(18) << left << "	no" << endl;
									}
								}
							}
						}
					}
					customers[i].checkqueue.pop();
					usleep(customers[i].check_time * 1000);
					clock_gettime(CLOCK_MONOTONIC, &current);
					elapsed = (current.tv_sec - created.tv_sec);
					elapsed += (current.tv_nsec - created.tv_nsec) / 1000000.0;
					customers[i].total_computation_time = customers[i].total_computation_time - customers[i].check_time;
					cout << "Elapsed: " << elapsed << "ms" << endl;
					if(elapsed < tempq.deadline) {
						cout << "Deadline of " << tempq.deadline << "ms met." << endl;
					}
					else
						cout << "Deadline of " << tempq.deadline << "ms missed by " << elapsed - tempq.deadline << "ms." << endl;
					cout << endl;
					if(sem_signal(semid, 0))
						exit(1);
				}
				else if(op == "pay") {
					if(sem_wait(semid, 0))
						exit(1);
					Pay tempq = customers[i].payqueue.front();
					for(int j = 0; j < tempq.roomcounter; j++) {
						int temproom = tempq.roomqueue.front();
						int startday = tempq.day;
						string name = tempq.name;
						int nameArraySpot;
						for(int x = 0; x < customerNamesCount; x++) {
							if(name == customerNames[x])
								nameArraySpot = x;
						}
						int startmonth = tempq.month;
						int days = tempq.number_of_days;
						for(int i = 0; i < days; i++) {
							int valid = shared_memory[temproom-1].date_booked[startmonth-1][startday-1][0];
							if((startday < daysInMonth[startmonth-1]) && (valid == cust)) {
								cout << "Customer " << cust << " (" << name <<  ") Paid for room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << endl;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][1] = 1;
								startday++;
							}
							else if((startday == daysInMonth[startmonth-1]) && (valid == cust)) {
								cout << "Customer " << cust << " (" << name <<  ") Paid for room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << endl;
								shared_memory[temproom-1].date_booked[startmonth-1][startday-1][1] = 1;
								startday = 1;
								startmonth++;
							}
							else {
								cout << "Customer " << cust << " (" << name <<  ") Could not pay for room " << temproom << " on " << startmonth << "/" << startday<< "/" << y << endl;
								if(startday < daysInMonth[startmonth-1]) {
									startday++;
								}
								else if(startday == daysInMonth[startmonth-1]) {
									startday = 1;
									startmonth++;
								}
							}
						}
						tempq.roomqueue.pop();
					}
					customers[i].payqueue.pop();
					usleep(customers[i].pay_time * 1000);
					clock_gettime(CLOCK_MONOTONIC, &current);
					elapsed = (current.tv_sec - created.tv_sec);
					elapsed += (current.tv_nsec - created.tv_nsec) / 1000000.0;
					customers[i].total_computation_time = customers[i].total_computation_time - customers[i].pay_time;
					cout << "Elapsed: " << elapsed << "ms" << endl;
					if(elapsed < tempq.deadline) {
						cout << "Deadline of " << tempq.deadline << "ms met." << endl;
					}
					else
						cout << "Deadline of " << tempq.deadline << "ms missed by " << elapsed - tempq.deadline << "ms." << endl;
					cout << endl;
					if(sem_signal(semid, 0))
						exit(1);
				}
				customers[i].transactions.pop();
			}
			laxity[i] = 99999999; //when a process is done, sets its laxity to a high number to avoid it from running again
			exit(0);
		}
		else { //is parent

		}
	}
	}

	//waits for each process to finish
	for (int i = 0; i < numOfCustomers; i++) {
        wait(NULL);
    }

	delete [] deadlines_array;

	if (shmdt(shared_memory) == -1) {
        perror("Error deattaching");
    }

	//deletes semaphores before exiting
	if(semctl(semid, 0, IPC_RMID) == -1) {
		perror("Error deleting semaphores");
		exit(1);
	}

	//deletes shared memory before exiting
	if(shmctl(shmid, IPC_RMID, NULL) == -1) {
		perror("Error deleting shared memory");
		exit(1);
	}
	//deletes shared memory before exiting
	if(shmctl(shmid2, IPC_RMID, NULL) == -1) {
		perror("Error deleting shared memory");
		exit(1);
	}

	if(choice == 2) {
		//deletes shared memory before exiting
		if(shmctl(shmid3, IPC_RMID, NULL) == -1) {
			perror("Error deleting shared memory");
			exit(1);
		}
	}

	return 0;
}


int sem_wait(int id, int semnum) {
	struct sembuf temp;
	temp.sem_num = semnum;
	temp.sem_op = -1; //decrements sem value (wait)
	temp.sem_flg = 0;
	if(semop(id, &temp, 1) == -1) {
		perror("Error waiting");
		return 1;
	}
	return 0;
}

int sem_signal(int id, int semnum) {
	struct sembuf temp;
	temp.sem_num = semnum;
	temp.sem_op = 1; //increments sem value (signal)
	temp.sem_flg = 0;
	if(semop(id, &temp, 1) == -1) {
		perror("Error signaling");
		return 1;
	}
	return 0;
}



