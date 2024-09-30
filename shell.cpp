#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <chrono>
#include <vector>
#include <string>
#include "Tokenizer.h"
#include <vector>
#include <fcntl.h>  // For open()
#include <cstring>


// all the basic colours for a shell prompt
#define RED     "\033[1;31m"
#define GREEN	"\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE	"\033[1;34m"
#define WHITE	"\033[1;37m"
#define NC      "\033[0m"

using namespace std;

int main () {
    // use dup(...) to perserve standard in and out
    int stdin = dup(0);   // Backup stdin
    int stdout = dup(1);  // Backup stdout

    char start[256];

    
    string user_input;
    string prev_dir = getcwd(nullptr, 0);

    typedef std::chrono::system_clock Clock;
    auto now = Clock::now();
    std::time_t now_c = Clock::to_time_t(now);
    struct tm *parts = std::localtime(&now_c);
    vector<string> months{"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    // get the name of the user 
    //      use getenv(...)
    char* name = getenv("USER");

    // get the previous working directory
    //      use getcwd(...)
    char* prevWorkDir = getcwd(start,sizeof(start));

    // create an array or vector to store the pids of the background processes
    vector<pid_t> pid_back_vector;

    // create an array or vector to store the pids of interrmediate processes
    vector<pid_t> pid_vector;

    int status;
    bool backprocess = true; //we need a thing to add background processes and set backprocess to true

    for (;;) {
        // need date/time, username, and absolute path to current dir
        //      time(...)
        //      ctime (...)
        //      getcwd(...)
        //cout << YELLOW << "Shell$" << NC << " ";
        
        //Displaying epic prompt
        now = Clock::now();
        now_c = Clock::to_time_t(now);
        parts = std::localtime(&now_c);

        cout << months[parts->tm_mon] << " " << parts->tm_mday << ":" << parts->tm_hour << ":" << parts->tm_min << name << ":" << prevWorkDir << "$ ";
        
        // get user inputted command
        string input;
        getline(cin, input);

        // reap background processes
        //  foreach (pid in packgroundPids)
        //      if (waitpid(pid, nullptr, WNOHANG)) // using WNOHANG makes it not wait
        //          print output
        //          packgroundPids.remove(pid)
        
        for (long unsigned int i = 0; i < pid_back_vector.size(); ++i  ) {
            if (waitpid(pid_back_vector[i], nullptr, WNOHANG)) {
                cout << pid_back_vector[i] << endl;
                pid_back_vector.erase(pid_back_vector.begin() + i);
                --i;
            }
        }


        string lower_input = input;
        for (char &ch : lower_input) {
            ch = tolower(ch);
        }

        // Handle exit command
        if (lower_input == "exit" && !backprocess) {  // print exit message and break out of infinite loop
            cout << RED << "Now exiting shell..." << endl << "Goodbye" << NC << endl;
            break;
        }

        // if (input == "exit") {  // print exit message and break out of infinite loop
        //     // reap all remaining background processes
        //     //  similar to previous reaping, but now wait for all to finish
        //     //      in loop use argument of 0 instead of WNOHANG

        //     cout << RED << "Now exiting shell..." << endl << "Goodbye" << NC << endl;
        //     break;
        // }

        if (lower_input == "exit" && backprocess) {  // print exit message and break out of infinite loop
            while (backprocess) {
                for (long unsigned int i = 0; i < pid_back_vector.size(); ++i  ) {
                    if (waitpid(pid_back_vector[i], &status, 0)) {
                        cout << pid_back_vector[i] << endl;
                        pid_back_vector.erase(pid_back_vector.begin() + i);
                        backprocess = true;
                        --i;
                    }
                    else { //if running
                        backprocess = false;
                        break;
                    }
                }
            }
            cout << RED << "Background processes complete: " << endl << "Now exiting shell:" << endl << "Goodbye" << NC << endl; 
        }

        // [optional] do dollar sign expansion

        // get tokenized commands from user input
        Tokenizer token(input);
        if (token.hasError()) {  // continue to next prompt if input had an error
            continue;
        }

        // // print out every command token-by-token on individual lines
        // // prints to cerr to avoid influencing autograder
        // for (auto cmd : token.commands) {
        //     for (auto str : cmd->args) {
        //         cerr << "|" << str << "| ";
        //     }
        //     if (cmd->hasInput()) {
        //         cerr << "in< " << cmd->in_file << " ";
        //     }
        //     if (cmd->hasOutput()) {
        //         cerr << "out> " << cmd->out_file << " ";
        //     }
        //     cerr << endl;
        // }

        // handle directory processing
        // if (token.commands.size() == 1 && token.commands.at(0)->args.at(0) == "cd") {
        //     // use getcwd(...) and chdir(...) to change the path

        //     // if the path is "-" then print the previous directory and go to it
        //     // otherwise just go to the specified directory
        // }
        //(tknr.commands.size() == 1 && tknr.commands.at(0)->args.at(0) == "cd") 
        
        if (token.commands[0]->args[0] == "cd") {
            string target_dir;
            string current_dir = getcwd(nullptr, 0);  // Save current directory for cd -

            if (token.commands[0]->args.size() > 1) {
                if (token.commands[0]->args[1] == "-") {
                    target_dir = prev_dir;
                    cout << target_dir << endl;  // Print the target directory for cd -
                } else {
                    target_dir = token.commands[0]->args[1];
                }
            } else {
                target_dir = getenv("HOME");  // Default to home directory
            }

            if (chdir(target_dir.c_str()) != 0) {
                perror("chdir");
            } else {
                prev_dir = current_dir;  // Update previous directory after successful change
            }
            continue;  // Skip the rest of the loop for built-in commands
        }

        // loop over piped commands
        for (size_t i = 0; i < token.commands.size(); i++) {
            // get the current command
            Command* currentCommand = token.commands.at(i);

            // pipe if the command is piped
            //  (if the number of commands is > 1)
            int pipefd[2];
            if (i < token.commands.size() - 1) {
                if (pipe(pipefd) == -1) {
                    perror("pipe");
                    exit(1);
                }
            }

            // fork to create child
            pid_t pid = fork();
            if (pid < 0) {  // error check
                perror("fork");
                exit(2);
            }

            // if (pid == 0) {  // if child, exec to run command
            //     // run single commands with no arguments
            //     // change to instead create an array from the vector of arguments
            //     char* args[] = {(char*) token.commands.at(0)->args.at(0).c_str(), nullptr};

            //     // duplicate the standard out for piping

            //     // do I/O redirection  
            //     // the Command class has methods hasInput(), hasOutput()
            //     //     and member variables in_file and out_file

            //     if (execvp(args[0], args) < 0) {  // error check
            //         perror("execvp");
            //         exit(2);
            //     }
            // }
            if (pid == 0) {  // if child, exec to run command
                // Create an array of arguments from the command's args vector
                char** args = new char*[currentCommand->args.size() + 1];
                for (size_t j = 0; j < currentCommand->args.size(); ++j) {
                    args[j] = const_cast<char*>(currentCommand->args.at(j).c_str());  // Convert std::string to char*
                }
                args[currentCommand->args.size()] = nullptr;  // Null-terminate the array

                // Handle input redirection
                if (currentCommand->hasInput()) {
                    int fd_in = open(currentCommand->in_file.c_str(), O_RDONLY);
                    if (fd_in < 0) {
                        perror("open input file");
                        exit(1);
                    }
                    dup2(fd_in, STDIN_FILENO);  // Redirect standard input
                    close(fd_in);  // Close the file descriptor
                }

                // Handle output redirection for piping
                if (i < token.commands.size() - 1) {  // if there is a next command
                    dup2(pipefd[1], STDOUT_FILENO);  // Redirect standard output to the pipe
                } else if (currentCommand->hasOutput()) {  // if there is an output file
                    int fd_out = open(currentCommand->out_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd_out < 0) {
                        perror("open output file");
                        exit(1);
                    }
                    dup2(fd_out, STDOUT_FILENO);  // Redirect standard output
                    close(fd_out);  // Close the file descriptor
                }

                // Execute the command
                if (execvp(args[0], args) < 0) {  // error check
                    perror("execvp");
                    exit(2);
                }

                delete[] args;  // Clean up dynamically allocated memory
            }

            else {  // if parent, wait for child to finish
                // if the current command is a background process then store it in the vector
                //  can use currentCommand.isBackground()
                if (currentCommand->isBackground()) {
                    // Store the PID of the background process
                    pid_back_vector.push_back(pid);
                }

                // else if the last command, wait on the last command pipline
                int status = 0;
                waitpid(pid, &status, 0);
                if (status > 1) {  // exit if child didn't exec properly
                    exit(status);
                }

                // else add the process to the intermediate processes
                else {
                    pid_vector.push_back(pid);
                }
                
                // dup standardin
                dup(0);
            }
        }

        

        // restore standard in and out using dup2(...)
        dup2(stdin, 0);  // Restore stdin   
        dup2(stdout, 1); // Restore stdout

        // reap intermediate processes
        for (long unsigned int i = 0; i < pid_vector.size(); ++i  ) {
            pid_vector.erase(pid_vector.begin() + i);
        }

    }
}

