# RMIT Operating System Principles Assignment 1 

Compile both the files, mmcopier.cpp and mscopier.cpp, with a single command, 
# make all

To delete all the executable files or objects for both the c++ files use the command,
# make clean

For Task 1 and Task 2 both, the destination file and destination directory needs to already exist to add in the arguements of the commands.

Command to run the Task 1 executable,
# ./mmcopier n <source_dir> <destination_dir>

Command to run the Task 2 executable,
# ./mscopier n <source_text_file.txt> <destination_text_file.txt> 

"mmcopier.cpp" 

Mutex for Synchronising Console Output

Line 14: Console_mutex, a variable declared by pthread_mutex_t, is used. To prevent log messages from overlapping or interleaving, this mutex is used to synchronise access to console output.

Lines 38–42: If the source file cannot be opened, an error message is printed before the console_mutex is locked using pthread_mutex_lock(&console_mutex). Afterwards, pthread_mutex_unlock(&console_mutex) is used to unlock it.

Lines 46–51: In a similar vein, if the destination file cannot be opened, the console_mutex is locked before an error message is printed. After the message is printed, it is unlocked.

Lines 56–58: After printing a success message confirming that a file has been successfully copied, the console_mutex is locked. After the message is printed, it is unlocked.

Line 151: After every thread has finished its operation, the console_mutex is destroyed by calling the pthread_mutex_destroy(&console_mutex) function.


"mscopier.cpp" 

Mutex for Buffer Synchronization

Line 15: Buffer_mutex, a pthread_mutex_t variable, is declared. In order to prevent race conditions, this mutex is used to synchronise access to the shared buffer, shared_buffer, ensuring that only one thread can access the buffer at a time.

Condition Variables for Management of Buffers

Line 18-19: Buffer_not_full and buffer_not_empty, two pthread_cond_t variables, are declared. The shared buffer's state is controlled by these condition variables, which enable threads to wait until the buffer reaches an appropriate state (such as not full or empty) before continuing.

Implementing Reader Threads

Line 34: To guarantee synchronised access to the shared buffer, the buffer_mutex is locked using pthread_mutex_lock(&buffer_mutex) at the start of the reader_thread function.

Lines 37-39: To avoid overflowing the buffer when adding new data, the pthread_cond_wait(&buffer_not_full, &buffer_mutex) function is utilised. By doing this, the reader thread is kept from adding data to a buffer that is already full.

Lines 41–47: All waiting writer threads are woken up by calling the pthread_cond_broadcast(&buffer_not_empty) function if the input file has been fully read and setting the reading_finished flag to true.

Line 50: To indicate that the buffer is not empty and to allow writer threads to begin processing the data, the pthread_cond_signal(&buffer_not_empty) function is called.

Line 51: To enable other threads to access the buffer, the buffer_mutex is unlocked using pthread_mutex_unlock(&buffer_mutex) following each crucial section.

Writer Threads Implementation

Line 62: To guarantee synchronised access to the shared buffer, the buffer_mutex is locked at the start of the writer_thread function.

Lines 65–70: Before trying to read data from the buffer, the pthread_cond_wait(&buffer_not_empty, &buffer_mutex) function is used to wait until the buffer is not empty. This stops writer threads from attempting to write in the absence of data.

Line 76: Reader threads can add more data to the buffer by using the pthread_cond_signal(&buffer_not_full) function, which is called to signal that the buffer is not full.

Line 77: After processing the buffer, the buffer_mutex is released to permit access to it by other threads.

Cleanup

Line 142: After every thread has finished its operation, the buffer_mutex is destroyed by calling the pthread_mutex_destroy(&buffer_mutex) function.

Lines 143–144: After all threads have finished their operations, the pthread_cond_destroy(&buffer_not_full) and pthread_cond_destroy(&buffer_not_empty) functions are called to destroy the condition variables.









