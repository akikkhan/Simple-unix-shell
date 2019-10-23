/*
****************************
Name: Khan, MD Akik		   *
Project: PA-1(Programming) *
Instructor: Feng Chen	   *
Class: cs7103-au18		   *
LogonID: cs710304		   *
****************************
*/

//list of header files
#include<iostream>
#include<sys/wait.h>
#include<unistd.h>
#include<string>
#include<cstring>
#include<cstdio>
#include<cstdlib>
#include<map>
#include<vector>
#include<set>
#include<algorithm>
#include<utility>
#include<fcntl.h>
#include<deque>

using namespace std;

//constants declaration
#define HISTAT_SIZE 10 //store top 10 most frequent commands
#define TOKEN_BUFFER_SIZE 64 //set buffer size
#define TOKEN_DELIM " \t\r\n\a" //tab, enter, newline, alert
#define PIPE_SYM "|" //symbol used to direct output from one program to another
#define REDIRECTION_SYM "><" //redirection to send output to a file
#define COMMAND_SIZE 30
#define NO_OF_COMMANDS 10 //initializing to 10


//forward declaration
string shl_read_input();
char ** shl_split_line(char *line, const char *delim);
int shl_check_redirect_pipe(char **args); //checks if ridirection used or pipe
int shl_execute(char **args); //executes command
int shl_cd(char **args);
int shl_exit(char **args);
int shl_num_of_specials();
int shl_num_of_builtins();
int printHistory(char **args);
int printHistat(char **args);
int shl_execute_redirect(char *command);
int shl_execute_pipe(char **commands);


//stores commands and their counts
map<string, int> histatCnt;

//displays latest 100 commands run by user
vector<string> histList;

//holder for history and histat
const char *special_str[] = {
	"history",
	"histat"
};

int (*special_func[]) (char **args) = {
	printHistory,
	printHistat
};

//holder for cd and exit
const char *builtin_str[] = {
	"cd",
	"exit"
};

int (*builtin_func[]) (char **args) = {
	shl_cd,
	shl_exit
};


int main() {

	string line_str;
	char *input = NULL;
	char **commands;
	int status = 0;


	do {
		printf("> ");
		line_str = shl_read_input(); //reads input

		if(line_str.length() < 1) {
			continue;
		}

		input = (char *) malloc(sizeof(char) * (line_str.length() + 1));
		strcpy(input, line_str.c_str());

		//splits command when pipe symbol used
		commands = shl_split_line(input, PIPE_SYM);

		//saves commands with redirection symbol
		status = shl_check_redirect_pipe(commands);

		//add to the list of executed commands
		histList.push_back(line_str);

		//counts the # of times commands executed
		if(histatCnt.find(line_str) != histatCnt.end()) {
			histatCnt[line_str] = histatCnt[line_str] + 1;
		} else {
			histatCnt[line_str] = 1;
		}
	} while(status > 0); //this ensures it's not reading i.e. status = 0

	return 0;
}

//reads user inputs from shell
string shl_read_input() {
	string input;
	getline(cin, input);
	return input;
}

//splits inputs based on delimiters
char ** shl_split_line(char *input, const char *delim = TOKEN_DELIM) {

	int buffer_size = TOKEN_BUFFER_SIZE;
	char **tokens = (char **) malloc(sizeof(char*) * buffer_size);
	char *token = strtok(input, delim);
	int index = 0;

	//adds inputs till everything is added
	while(token != NULL) {
		tokens[index++] = token;

		//if index reaches buffer size limit, add extra bits
		if(index == buffer_size) {
			buffer_size += TOKEN_BUFFER_SIZE;
			tokens = (char**) realloc(tokens, sizeof(char*) * buffer_size);
			if(tokens == NULL) {
				exit(1);
			}
		}

		token = strtok(NULL, delim);
	}

	//empties token before returning
	tokens[index] = NULL;
	return tokens;
}

//checks if there's any redirection symbol or pipe symbol in inputs
int shl_check_redirect_pipe(char **commands) {

	//no input given
	if(commands[0] == NULL) {
		return 1;
	}

	if(commands[1] == NULL) {

		//checks if only ridirection used without any commands
		if(strchr(commands[0], '>') != NULL || strchr(commands[0], '<') != NULL) {
			return shl_execute_redirect(commands[0]);
		}

		char **args = shl_split_line(commands[0]);
		return shl_execute(args);

	} else {	//pipe used
		return shl_execute_pipe(commands);

	}
}


//executes commands with ridirection symbol
int shl_execute_redirect(char *command) {

	char command_copy[strlen(command) + 1];
	strcpy(command_copy, command);
	char **splitted_command = shl_split_line(command, REDIRECTION_SYM);
	char *commandName = splitted_command[0];
	char *input_file = NULL;
	char *output_file = NULL;
	int j = 1;

	//find I/O file name
	for(int i = 0; command_copy[i] && splitted_command[j]; i++) {

		//reads commands from input file
		if(command_copy[i] == '<') {

			input_file = splitted_command[j++];

		} else if(command_copy[i] == '>') {

			//writes commands to output file
			output_file = splitted_command[j++];
		}
	}

	//no filename given
	if(input_file == NULL && output_file == NULL) {
		printf("shell: syntex error\n");
		return 1;
	}

	int std_in_copy;
	int std_out_copy;
	int input_file_dir;
	int output_file_dir;

	//redirect STD_IN to input_file
	if(input_file != NULL) {

		//creates a copy of the read-file descriptor
		std_in_copy = dup(STDIN_FILENO);

		//closes the actual file descriptor #
		close(STDIN_FILENO);

		input_file_dir = open(input_file, O_RDONLY);

		if(input_file_dir < 0) {
			perror(input_file);

			//since there's no file, it'll reassign the old file descriptor
			dup2(std_in_copy, STDIN_FILENO);

			//closes the copy fd
			close(std_in_copy);

			return 1;
		}
	}

	//redirect STD_OUT to output_file
	if(output_file != NULL) {

		//similar to input file method
		std_out_copy = dup(STDOUT_FILENO);
		close(STDOUT_FILENO);
		output_file_dir = open(output_file, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

		if(output_file_dir < 0) {
			perror(output_file);
			dup2(std_out_copy, STDOUT_FILENO);
			close(std_out_copy);
			return 1;
		}
	}

	pid_t pid = fork();

	if(pid == 0) { //child process

		char **command_args = shl_split_line(commandName);

		//calls special function and executes special commands
		for(int i = 0; i < shl_num_of_specials(); i++) {

			if(strcmp(special_str[i], command_args[0]) == 0) {

				special_func[i](command_args);
				exit(EXIT_SUCCESS);
			}
		}

		if(execvp(command_args[0], command_args) == -1) {
			perror(command_args[0]);
		}

		exit(EXIT_FAILURE);
	}

	//restore STD_IN
	if(input_file != NULL) {

		close(input_file_dir);

		//returns to initial state, assigning 0 to read
		dup2(std_in_copy, 0);

		close(std_in_copy);
	}

	//restore STD_OUT
	if(output_file != NULL) {

		close(output_file_dir);

		//returns to intial state, assigning 1 to write
		dup2(std_out_copy, 1);

		close(std_out_copy);
	}

	int status;

	//waits for the child process to finish writing
	waitpid(pid, &status, WUNTRACED);

	return 1;
}

//length of commands
int getLength(char **stringArray) {

	int length = 0;
	while(stringArray[length]) {
		length++;
	}
	return length;
}

//implementing pipe symbol
int shl_execute_pipe(char **commands) {

	int num_of_commands = getLength(commands);
	int num_of_pipes = num_of_commands - 1;
	int pipe_descriptor[num_of_pipes][2];//one for read, one for write

	for(int i = 0; i < num_of_pipes; i++) {
		pipe(pipe_descriptor[i]);
	}

	pid_t child_id[num_of_commands];

	for(int i = 0; commands[i] != NULL; i++) {

		child_id[i] = fork();

		//creates another child process
		if(child_id[i] == 0) {

			if(i != num_of_commands - 1) {
				//pipe_descriptor for writing end
				dup2(pipe_descriptor[i][1], STDOUT_FILENO);
			}

			for(int j = i + 1; j < num_of_pipes; j++) {
				//close pipe_descriptor's writing end
				close(pipe_descriptor[j][1]);
			}

			if(i > 0) {
				//pipe_descriptor for reading end
				dup2(pipe_descriptor[i-1][0], STDIN_FILENO);
			}

			for(int j = 0; j < num_of_pipes; j++) {

				if(j != i - 1) {
					//close pipe_descriptor's reading end
					close(pipe_descriptor[j][0]);
				}
			}

			char **args = shl_split_line(commands[i]);

			//check for errors
			if(execvp(args[0], args) == -1) {
				perror(args[0]);
			}

			exit(EXIT_FAILURE);

		} else {

			if(i < num_of_pipes) {
				close(pipe_descriptor[i][1]);
			}

			int status;
			waitpid(child_id[i], &status, WUNTRACED);
		}
	}

	for(int i = 0; i < num_of_pipes; i++) {
		//close all pipe_descriptors reading ends
		close(pipe_descriptor[i][0]);
	}

	return 1;
}

int shl_execute(char **args) {

	//executes builtin commands in the parent process if any command matches
	for(int i = 0; i < shl_num_of_builtins(); i++) {

		if(strcmp(builtin_str[i], args[0]) == 0) {

			return builtin_func[i](args);
		}
	}

	pid_t pid, wpid;
	int status;
	pid = fork();

	if(pid == 0) {
		//child process

		//executes special commands in the child process if any command matches
		for(int i = 0; i < shl_num_of_specials(); i++) {

			if(strcmp(special_str[i], args[0]) == 0) {

				special_func[i](args);
				exit(EXIT_SUCCESS);
			}
		}

		if(execvp(args[0], args) == -1) {

			perror(args[0]);
		}

		exit(EXIT_FAILURE);

	} else if(pid < 0) {
		perror(args[0]);

	} else {

		do {
			wpid = waitpid(pid, &status, WUNTRACED);
		} while(!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	return 1;
}

//counts special commands
int shl_num_of_specials() {

	static int size = sizeof(special_str)/sizeof(char *);
	return size;
}

//counts number of builtin commands
int shl_num_of_builtins() {

	static int size = sizeof(builtin_str)/sizeof(char *);
	return size;
}

//runs when cd typed
int shl_cd(char **args) {

	if(args[1] == NULL) {
		fprintf(stderr, "lsh: expected argument to \"cd\"\n");
	} else {
		if(chdir(args[1]) != 0) {
			perror(args[0]);
		}
	}
	return 1;
}

//executes for exit
int shl_exit(char **args) {
	return 0;
}

//splits commands based on pipe symbol
char** split_command(char *input) {
    char **commands = (char**) malloc(sizeof(char*) * NO_OF_COMMANDS);
    char *command = NULL;
    int commands_index = 0;
    int index, i;
    for(i = 0; input[i] && input[i] != '\n'; i++) {

		//no input, set index to 0
        if(command == NULL) {
            command = (char*) malloc(sizeof(char) * COMMAND_SIZE);
            index = 0;
        }

		//runs when pipe symbol found in commands
        if(input[i] == '|') {
            command[index] = '\0';
            commands[commands_index++] = command;
            command = NULL;
        } else {
            command[index++] = input[i];
        }
    }

    if(i > 0) {
        command[index] = '\0';
        commands[commands_index++] = command;
    }
    command[index] = '\0'; //adds \0 bytes
    return commands;
}


int printHistory(char **args) {

	int cnt = 100;
	if(histList.size() < 100)
		cnt = histList.size();

	int i = histList.size() - 1;
	//print everything from histList till 100th command
	while(cnt) {
		cout << histList[i] << endl;
		cnt--;
		i--;
	}
	return 1;
}

bool comp(pair<string, int> a, pair<string, int> b){
	return a.second > b.second;
}

//prints counts of commands
int printHistat(char **args) {

    vector<pair<string, int> > hisstatList;
    map<string, int >::iterator itr;

	for(itr = histatCnt.begin(); itr != histatCnt.end(); itr++) {
		hisstatList.push_back(make_pair((itr -> first), (itr -> second)));
	}

	sort(hisstatList.begin(), hisstatList.end(), comp);

	int cnt = 10;
	if(hisstatList.size() < cnt)
		cnt = hisstatList.size();

	int index = 0;
	while(cnt) {
		cout << hisstatList[index].first << ' ' << hisstatList[index].second << endl;
		cnt--;
		index++;
	}

	return 1;
}
