#include <iostream>
#include <fstream>
#include <queue>
#include <vector>
#include <pthread.h>
#include <string>

const int MAX_QUEUE_SIZE = 20;

// Shared queue to hold lines read from the file
std::queue<std::string> shared_queue;

// Mutex to protect access to the shared queue
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

// Condition variables to signal when the queue is not full and not empty
pthread_cond_t queue_not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t queue_not_empty = PTHREAD_COND_INITIALIZER;

bool reading_done = false;  // Flag to indicate that all reading is complete

struct ThreadArgs {
    std::ifstream* source_file;
    std::ofstream* destination_file;
};

// Reader thread function
void* reader_thread(void* arg) {
    ThreadArgs* args = static_cast<ThreadArgs*>(arg);
    std::string line;

    while (true) {
        pthread_mutex_lock(&queue_mutex);

        // Wait until there is space in the queue
        while (shared_queue.size() >= MAX_QUEUE_SIZE) {
            pthread_cond_wait(&queue_not_full, &queue_mutex);
        }

        if (!std::getline(*args->source_file, line)) {
            // If reading is done, set the flag and signal writers
            reading_done = true;
            pthread_cond_broadcast(&queue_not_empty);  // Wake up the writer
            pthread_mutex_unlock(&queue_mutex);
            break;
        }

        shared_queue.push(line);
        pthread_cond_signal(&queue_not_empty); // Signal that the queue is not empty
        pthread_mutex_unlock(&queue_mutex);
    }

    return nullptr;
}

// Writer thread function (single writer to maintain order)
void* writer_thread(void* arg) {
    ThreadArgs* args = static_cast<ThreadArgs*>(arg);

    while (true) {
        pthread_mutex_lock(&queue_mutex);

        // Wait until there is something in the queue
        while (shared_queue.empty()) {
            if (reading_done) {
                pthread_mutex_unlock(&queue_mutex);
                return nullptr;  // Exit the writer thread if reading is done and queue is empty
            }
            pthread_cond_wait(&queue_not_empty, &queue_mutex);
        }

        std::string line = shared_queue.front();
        shared_queue.pop();

        pthread_cond_signal(&queue_not_full); // Signal that the queue is not full
        pthread_mutex_unlock(&queue_mutex);

        // Check if this is the last line
        if (shared_queue.empty() && reading_done) {
            *args->destination_file << line;  // Write the last line without a newline
        } else {
            *args->destination_file << line << '\n';  // Write the line with a newline
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <n> <source_file> <destination_file>" << std::endl;
        return 1;
    }

    int n = std::stoi(argv[1]);
    if (n < 2 || n > 10) {
        std::cerr << "Error: n must be between 2 and 10" << std::endl;
        return 1;
    }

    std::ifstream source_file(argv[2]);
    if (!source_file.is_open()) {
        std::cerr << "Error: Could not open source file " << argv[2] << std::endl;
        return 1;
    }

    std::ofstream destination_file(argv[3]);
    if (!destination_file.is_open()) {
        std::cerr << "Error: Could not open destination file " << argv[3] << std::endl;
        return 1;
    }

    ThreadArgs args = { &source_file, &destination_file };
    std::vector<pthread_t> readers(n);
    pthread_t writer;  // Single writer thread

    // Create reader threads
    for (int i = 0; i < n; ++i) {
        pthread_create(&readers[i], nullptr, reader_thread, &args);
    }

    // Create a single writer thread
    pthread_create(&writer, nullptr, writer_thread, &args);

    // Wait for all reader threads to finish
    for (int i = 0; i < n; ++i) {
        pthread_join(readers[i], nullptr);
    }

    // Wait for the writer thread to finish
    pthread_join(writer, nullptr);

    source_file.close();
    destination_file.close();

    // Clean up mutexes and condition variables
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&queue_not_full);
    pthread_cond_destroy(&queue_not_empty);

    std::cout << "File copied successfully using " << n << " readers and 1 writer." << std::endl;

    return 0;
}
