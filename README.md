# Parallel File Search Using Threads

## About the Project

**Parallel File Search Using Threads** is a file-search application developed in C using POSIX threads (`pthread`) on Ubuntu/Linux. It helps users find files by name and search for specific words or phrases inside files across directories and subdirectories.

The application uses multiple worker threads to process search tasks, helping demonstrate how parallel processing works in operating systems. It also uses mutex locks to protect shared data and keep search results organized.

## Features

- **Filename Search:** Find files and directories by entering a filename or part of a name.
- **Text Search:** Search for words or phrases inside files.
- **Recursive Directory Search:** Search through folders and their subfolders.
- **Multithreading:** Choose between 1 and 16 worker threads.
- **Thread Safety:** Use mutex locks to protect shared counters and terminal output.
- **Search Summary:** Display the number of files checked, matches found, and threads used.

## Technologies Used

- C Programming Language
- POSIX Threads (`pthread`)
- GCC Compiler
- Ubuntu/Linux
- Linux file and directory APIs

## How It Works

1. The user enters the folder path to search.
2. The user selects filename search or text search.
3. The search term and number of threads are provided.
4. Worker threads process assigned search tasks.
5. Matching results are displayed, followed by a search summary.

## Installation and Execution

Install the required compiler tools on Ubuntu:

```bash
sudo apt update
sudo apt install build-essential
```

Compile the program:

```bash
gcc -Wall -Wextra -pthread main.c -o parallel_search
```

Run the application:

```bash
./parallel_search
```

## Sample Results

In one demonstration run, the application searched for `main.c` using five threads and reported:

- **Files checked:** 102
- **Matches found:** 7
- **Threads used:** 5

Results may vary depending on the selected directory and search term.

## Future Enhancements

- Search by file extension and file size.
- Find hidden files and configuration files.
- Detect duplicate files.
- Save search history and export results.
- Compare search performance using different thread counts.

## Team Members

| Name | Student ID |
|---|---|
| Venkata Sai Krishna | 2520030027 |
| Pujith Pavan Kumar | 2520030029 |
| Ajai Manikanta | 2520030320 |
| Bandla Vinay | 2520030437 |

**Department:** Computer Science and Engineering  
**Institution:** K L (Deemed to be) University  
**Academic Year:** 2026–2027

## Conclusion

This project demonstrates file handling, directory traversal, and multithreading using C and POSIX threads. It provides a practical way to explore how multiple threads can share search tasks and how synchronization helps manage shared data safely.