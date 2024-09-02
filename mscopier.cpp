#include <iostream>
#include <fstream>
#include <queue>
#include <vector>
#include <pthread.h>
#include <string>
#include <cstdlib> // For std::strtol

const int MAX_BUFFER_SIZE = 20;  // Maximum capacity of the shared buffer

// Shared buffer to store lines read from the input file
std::queue<std::string> shared_buffer;

// Mutex to ensure synchronized access to the shared buffer
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

// Condition variables to manage the buffer's fullness and emptiness
pthread_cond_t buffer_not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t buffer_not_empty = PTHREAD_COND_INITIALIZER;

bool reading_finished = false;  // Flag to indicate when reading from the input file is complete

struct FileHandles {
    std::ifstream* input_stream;
    std::ofstream* output_stream;
};

// Function executed by the reader threads to read lines from the input file
void* reader_thread(void* arg) {
    FileHandles* file_handles = static_cast<FileHandles*>(arg);
    std::string line;

    while (true) {
        pthread_mutex_lock(&buffer_mutex);

        // Wait if the buffer is full
        while (shared_buffer.size() >= MAX_BUFFER_SIZE) {
            pthread_cond_wait(&buffer_not_full, &buffer_mutex);
        }

        if (!std::getline(*file_handles->input_stream, line)) {
            // If reading is finished, set the flag and notify all writer threads
            reading_finished = true;
            pthread_cond_broadcast(&buffer_not_empty);  // Wake up all writer threads
            pthread_mutex_unlock(&buffer_mutex);
            break;
        }

        shared_buffer.push(line);  // Add the line to the buffer
        pthread_cond_signal(&buffer_not_empty);  // Signal that the buffer is not empty
        pthread_mutex_unlock(&buffer_mutex);
    }

    return nullptr;
}

// Function executed by the writer threads to write lines to the output file
void* writer_thread(void* arg) {
    FileHandles* file_handles = static_cast<FileHandles*>(arg);

    while (true) {
        pthread_mutex_lock(&buffer_mutex);

        // Wait if the buffer is empty
        while (shared_buffer.empty()) {
            if (reading_finished) {
                pthread_mutex_unlock(&buffer_mutex);
                return nullptr;  // Exit the writer thread if reading is finished and the buffer is empty
            }
            pthread_cond_wait(&buffer_not_empty, &buffer_mutex);
        }

        std::string line = shared_buffer.front();  // Get the front line from the buffer
        shared_buffer.pop();  // Remove the line from the buffer

        pthread_cond_signal(&buffer_not_full);  // Signal that the buffer is not full
        pthread_mutex_unlock(&buffer_mutex);

        // Write the line to the output file
        if (file_handles->output_stream->tellp() > 0) {
            *file_handles->output_stream << '\n';
        }
        *file_handles->output_stream << line;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <thread_count> <input_file> <output_file>" << std::endl;
        return 1;
    }

    char* endptr;
    int thread_count = std::strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || thread_count < 2 || thread_count > 10) {
        std::cerr << "Error: thread_count must be a number between 2 and 10" << std::endl;
        return 1;
    }

    std::ifstream input_file(argv[2]);
    if (!input_file.is_open()) {
        std::cerr << "Error: Could not open input file " << argv[2] << std::endl;
        return 1;
    }

    std::ofstream output_file(argv[3]);
    if (!output_file.is_open()) {
        std::cerr << "Error: Could not open output file " << argv[3] << std::endl;
        return 1;
    }

    FileHandles file_handles = { &input_file, &output_file };
    std::vector<pthread_t> readers;
    std::vector<pthread_t> writers;
    readers.resize(thread_count);
    writers.resize(thread_count);

    // Create the reader threads
    for (int i = 0; i < thread_count; ++i) {
        pthread_create(&readers[i], nullptr, reader_thread, &file_handles);
    }

    // Create the writer threads
    for (int i = 0; i < thread_count; ++i) {
        pthread_create(&writers[i], nullptr, writer_thread, &file_handles);
    }

    // Wait for all reader threads to finish
    for (int i = 0; i < thread_count; ++i) {
        pthread_join(readers[i], nullptr);
    }

    // Wait for all writer threads to finish
    for (int i = 0; i < thread_count; ++i) {
        pthread_join(writers[i], nullptr);
    }

    input_file.close();
    output_file.close();

    // Clean up the mutex and condition variables
    pthread_mutex_destroy(&buffer_mutex);
    pthread_cond_destroy(&buffer_not_full);
    pthread_cond_destroy(&buffer_not_empty);

    std::cout << "File copied successfully using " << thread_count << " readers and " << thread_count << " writers." << std::endl;

    return 0;
}
