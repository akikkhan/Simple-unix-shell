We implemented a simple *shell* program by using **fork()/execv()** system calls. This shell supports:
* Basic bash-like shell functions, such as loading and running a program
* Advanced features:
  * **pipe (|)** to direct output from one program as input to another program
  * **I/O redirection (>)** to send output to another file
* Special commands:
  * **“history”**, which *displays* the *latest 100 commands* typed by the user
  * **“histat”**, which *displays* the *top 10* most frequently typed commands in the descending order

Programming language used in this project was **C++**.
