# Welcome to Homework 0!

For these questions you'll need the mini course and  "Linux-In-TheBrowser" virtual machine (yes it really does run in a web page on your machine using javascript!) at -

https://cs-education.github.io/sys/

Let's take a look at some C code (with apologies to a well known song)-
```C
// An array to hold the following bytes. "q" will hold the address of where those bytes are.
// The [] mean set aside some space and copy these bytes into teh array array
char q[] = "Do you wanna build a C99 program?";

// This will be fun if our code has the word 'or' in later...
#define or "go debugging with gdb?"

// sizeof is not the same as strlen. You need to know how to use these correctly, including why you probably want strlen+1

static unsigned int i = sizeof(or) != strlen(or);

// Reading backwards, ptr is a pointer to a character. (It holds the address of the first byte of that string constant)
char* ptr = "lathe"; 

// Print something out
size_t come = fprintf(stdout,"%s door", ptr+2);

// Challenge: Why is the value of away equal to 1?
int away = ! (int) * "";


// Some system programming - ask for some virtual memory

int* shared = mmap(NULL, sizeof(int*), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
munmap(shared,sizeof(int*));

// Now clone our process and run other programs
if(!fork()) { execlp("man","man","-3","ftell", (char*)0); perror("failed"); }
if(!fork()) { execlp("make","make", "snowman", (char*)0); execlp("make","make", (char*)0)); }

// Let's get out of it?
exit(0);
```

## So you want to master System Programming? And get a better grade than B?
```C
int main(int argc, char** argv) {
	puts("Great! We have plenty of useful resources for you, but it's up to you to");
	puts(" be an active learner and learn how to solve problems and debug code.");
	puts("Bring your near-completed answers the problems below");
	puts(" to the first lab to show that you've been working on this.");
	printf("A few \"don't knows\" or \"unsure\" is fine for lab 1.\n"); 
	puts("Warning: you and your peers will work hard in this class.");
	puts("This is not CS225; you will be pushed much harder to");
	puts(" work things out on your own.");
	fprintf(stdout,"This homework is a stepping stone to all future assignments.\n");
	char p[] = "So, you will want to clear up any confusions or misconceptions.\n";
	write(1, p, strlen(p) );
	char buffer[1024];
	sprintf(buffer,"For grading purposes, this homework 0 will be graded as part of your lab %d work.\n", 1);
	write(1, buffer, strlen(buffer));
	printf("Press Return to continue\n");
	read(0, buffer, sizeof(buffer));
	return 0;
}
```
## Watch the videos and write up your answers to the following questions. For now just save them in a text document - we'll explain how to hand them later. 
They will be due in the second week of class.

**Important!**

The virtual machine-in-your-browser and the videos you need for HW0 are here:

https://cs-education.github.io/sys/

The coursebook:

https://cs341.cs.illinois.edu/coursebook/index.html

Questions? Comments? Use Ed: (you'll need to accept the sign up link I sent you)
https://edstem.org/

The in-browser virtual machine runs entirely in Javascript and is fastest in Chrome. Note the VM and any code you write is reset when you reload the page, *so copy your code to a separate document.* The post-video challenges (e.g. Haiku poem) are not part of homework 0 but you learn the most by doing (rather than just passively watching) - so we suggest you have some fun with each end-of-video challenge.

HW0 questions are below. Copy your answers into a text document (which the course staff will grade later) because you'll need to submit them later in the course. More information will be in the first lab.

## Chapter 1

In which our intrepid hero battles standard out, standard error, file descriptors and writing to files.

### Hello, World! (system call style)
1. Write a program that uses `write()` to print out "Hi! My name is `<Your Name>`".
```C
#include <unistd.h>
int main() {
	write(1, "Hi! My name is Andrew", 22);
	return 0;
}
```
### Hello, Standard Error Stream!
2. Write a function to print out a triangle of height `n` to standard error.
   - Your function should have the signature `void write_triangle(int n)` and should use `write()`.
   - The triangle should look like this, for n = 3:
   ```C
   *
   **
   ***
   ```
```C
#include <unistd.h>
void write_triangle(int n) {
	char line[n];
	int i;
	for (i = 1; i <= n; i++) {
		// fill the line with asterisks up to the current row number
		int j;
		for (j = 0; j < i; j++) {
			line[j] = '*';
		}
		// add a newline character at the end of the row
		line[i] = '\n';
		
		// write the line to standard error
		write(STDERR_FILENO, line, i + 1);
	}
}
int main() {
	write_triangle(3);
	return 0;
}
```
### Writing to files
3. Take your program from "Hello, World!" modify it write to a file called `hello_world.txt`.
   - Make sure to to use correct flags and a correct mode for `open()` (`man 2 open` is your friend).
```C
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
int main() {
	mode_t mode = S_IRUSR | S_IWUSR;
	int file = open("hello_world.txt", O_CREAT | O_TRUNC| O_RDWR, mode);
	write(file, "Hi! My name is Andrew", 22);
	close(file);
	return 0;
}
```
### Not everything is a system call
4. Take your program from "Writing to files" and replace `write()` with `printf()`.
   - Make sure to print to the file instead of standard out!
```C
#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
int main() {
	int original_stdout = dup(STDOUT_FILENO);
	close(1);
	mode_t mode = S_IRUSR | S_IWUSR;
	int file = open("hello_world.txt", O_CREAT | O_TRUNC| O_RDWR, mode);
	printf("Hi! My name is Andrew\n");
	close(file);
	dup2(original_stdout, STDOUT_FILENO);
	return 0;
}
```
5. What are some differences between `write()` and `printf()`?

write() is a system call provided by the OS while printf is a standard C library function

write() is unbuffered while printf is buffered which is flushed only when full, encounters \n or when fflush is called

printf can format string with placeholders while write just writes raw bytes to files

## Chapter 2

Sizing up C types and their limits, `int` and `char` arrays, and incrementing pointers

### Not all bytes are 8 bits?
1. How many bits are there in a byte?

At least 8 bits and it is normally 8 bits. This may be more for different architectures

2. How many bytes are there in a `char`?

Always 1 Byte

3. How many bytes the following are on your machine?
   - `int`, `double`, `float`, `long`, and `long long`

int: 4 Bytes
double: 8 Bytes
float: 4 Bytes
long: 4 Bytes
long long: 8 Bytes

### Follow the int pointer
4. On a machine with 8 byte integers:
```C
int main(){
    int data[8];
} 
```
If the address of data is `0x7fbd9d40`, then what is the address of `data+2`?

0x7fbd9d48

5. What is `data[3]` equivalent to in C?
   - Hint: what does C convert `data[3]` to before dereferencing the address?

it is equivalent to *(data+3) and 3[data]

### `sizeof` character arrays, incrementing pointers
  
Remember, the type of a string constant `"abc"` is an array.

6. Why does this segfault?
```C
char *ptr = "hello";
*ptr = 'J';
```
It causes a segmentation fault as "hello" is read only memory and we are attempting to modify it

7. What does `sizeof("Hello\0World")` return?

12

8. What does `strlen("Hello\0World")` return?

5

9. Give an example of X such that `sizeof(X)` is 3.

"ab"

10. Give an example of Y such that `sizeof(Y)` might be 4 or 8 depending on the machine.

Y = long

## Chapter 3

Program arguments, environment variables, and working with character arrays (strings)

### Program arguments, `argc`, `argv`
1. What are two ways to find the length of `argv`?

Look at the value of argc or iterate through argv till you reach a null pointer

2. What does `argv[0]` represent?

The program name

### Environment Variables
3. Where are the pointers to environment variables stored (on the stack, the heap, somewhere else)?

It's stored somewhere else that's not the stack or the heap and is a region of memory that 
is dedicated for environment variables. The location of this region depends on the OS.

### String searching (strings are just char arrays)
4. On a machine where pointers are 8 bytes, and with the following code:
```C
char *ptr = "Hello";
char array[] = "Hello";
```
What are the values of `sizeof(ptr)` and `sizeof(array)`? Why?

sizeof(ptr) will return 8 bytes
sizeof(array) will return 5 bytes

### Lifetime of automatic variables

5. What data structure manages the lifetime of automatic variables?

The stack data structure

## Chapter 4

Heap and stack memory, and working with structs

### Memory allocation using `malloc`, the heap, and time
1. If I want to use data after the lifetime of the function it was created in ends, where should I put it? How do I put it there?

I should put it in the heap using malloc

2. What are the differences between heap and stack memory?

Stack is static memory allocation, meant for local variables and function call management, 
automatically managed.
Heap is used for dynamic memory allocation, variable are manually allocated and deallocated
by the user

3. Are there other kinds of memory in a process?

There is also the: text segment, data segment, caches and registers

4. Fill in the blank: "In a good C program, for every malloc, there is a ___".

free

### Heap allocation gotchas
5. What is one reason `malloc` can fail?

Insufficient memory, if it lacks contiguous memory to allocate it will return NULL

6. What are some differences between `time()` and `ctime()`?

time returns a time_t variable while ctime returns a char* that is human readable
time's prupose is to obtain current time for computation
ctime is for obtaining a human readable version

7. What is wrong with this code snippet?
```C
free(ptr);
free(ptr);
```

it is attempting to double free the same memory

8. What is wrong with this code snippet?
```C
free(ptr);
printf("%s\n", ptr);
```

it is using freed memory which may be invalid or is reused for another purpose

9. How can one avoid the previous two mistakes? 

once you free a ptr, set it to NULL

### `struct`, `typedef`s, and a linked list
10. Create a `struct` that represents a `Person`. Then make a `typedef`, so that `struct Person` can be replaced with a single word. A person should contain the following information: their name (a string), their age (an integer), and a list of their friends (stored as a pointer to an array of pointers to `Person`s).

```C
struct Person {
    char* name;
    int age;
    struct Person** friends;
};

typedef struct Person Human;
```

11. Now, make two persons on the heap, "Agent Smith" and "Sonny Moore", who are 128 and 256 years old respectively and are friends with each other.

```C
int main() {
    Human *agentSmith = malloc(sizeof(Human));
    Human *soonyMoore = malloc(sizeof(Human));

    agentSmith->name = "Agent Smith";
    agentSmith->age = 128;
    agentSmith->friends = &sonnyMoore;

    sonnyMoore->name = "Sonny Moore";
    sonnyMoore->age = 256;
    sonnyMoore->friends = &agentSmith;

    free(agentSmith);
    free(sonnyMoore);

    return 0;
}
```

### Duplicating strings, memory allocation and deallocation of structures
Create functions to create and destroy a Person (Person's and their names should live on the heap).
12. `create()` should take a name and age. The name should be copied onto the heap. Use malloc to reserve sufficient memory for everyone having up to ten friends. Be sure initialize all fields (why?).

```C
Human *create(char *name, int age) {
    Human *p = malloc(sizeof(Human));
    p->name = strdup(name);
    p->age = age;
    p->friends = malloc(sizeof(Human*) * 10);
    int i;
    for (i = 0; i < 10; i++) {
        p->friends[i]= NULL;
    }
    return p;
}
```
Initialising of all fields to prevent undefined variables

13. `destroy()` should free up not only the memory of the person struct, but also free all of its attributes that are stored on the heap. Destroying one person should not destroy any others.

```C
void destroy(Human *p) {
    free(p->name);
    free(p->friends);
    free(p);
}
```

## Chapter 5 

Text input and output and parsing using `getchar`, `gets`, and `getline`.

### Reading characters, trouble with gets
1. What functions can be used for getting characters from `stdin` and writing them to `stdout`?

getchar() & putchar() to get and write respectively

2. Name one issue with `gets()`.

gets() does not check bounds and may be vulnerable to buffer overflow attacks

### Introducing `sscanf` and friends
3. Write code that parses the string "Hello 5 World" and initializes 3 variables to "Hello", 5, and "World".

```C
char str1[10], str2[10];
int num;
sscanf("Hello 5 World", "%s %d %s", str1, &num, str2);
```

### `getline` is useful
4. What does one need to define before including `getline()`?

_GNU_SOURCE

5. Write a C program to print out the content of a file line-by-line using `getline()`.

```C
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE *fp = fopen("filename.txt", "r");
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    if (fp == NULL) {
        perror("Error opening file");
        return 0;
    }

    while((read = getline(&line, &len, fp)) != -1) {
        printf("%s", line);
    }
    free(line);
    fclose(fp);
    return 0;
}
```

## C Development

These are general tips for compiling and developing using a compiler and git. Some web searches will be useful here

1. What compiler flag is used to generate a debug build?

-g

2. You modify the Makefile to generate debug builds and type `make` again. Explain why this is insufficient to generate a new build.

just modifying the Makefile may not trigger a rebuild if the source files haven't been changed. Make checks file timestamps to decide if recompilation is needed.

3. Are tabs or spaces used to indent the commands after the rule in a Makefile?

commands must be indented with a tab character

4. What does `git commit` do? What's a `sha` in the context of git?

git commit records changes to the repository. SHA is a unique identifier for each commit

5. What does `git log` show you?

dipslays commit history

6. What does `git status` tell you and how would the contents of `.gitignore` change its output?

git status shows the working tree status, .gitignore may exluude specified files from the untracked
fiels list

7. What does `git push` do? Why is it not just sufficient to commit with `git commit -m 'fixed all bugs' `?

git push uploads local repository content to a remote repository, commit only saves changes locally

8. What does a non-fast-forward error `git push` reject mean? What is the most common way of dealing with this?

It means that the remote branch has progressed since the last pull. Commonlu resolved by first pulling then pushing again


## Optional (Just for fun)
- Convert your a song lyrics into System Programming and C code and share on Ed.
- Find, in your opinion, the best and worst C code on the web and post the link to Ed.
- Write a short C program with a deliberate subtle C bug and post it on Ed to see if others can spot your bug.
- Do you have any cool/disastrous system programming bugs you've heard about? Feel free to share with your peers and the course staff on Ed.
