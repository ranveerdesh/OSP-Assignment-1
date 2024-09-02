#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <regex>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <cstring>
#include <errno.h>

// Mutex for synchronizing console output (optional but good for thread-safe logging)
pthread_mutex_t console_mutex;

// Structure to hold source and destination file paths
struct FilePair {
    std::string source;
    std::string destination;
};

// Function to extract numeric part from filename for sorting
int extract_number(const std::string& filename) {
    std::regex number_regex("(\\d+)");
    std::smatch match;
    if (std::regex_search(filename, match, number_regex)) {
        return std::stoi(match.str());
    }
    return 0; // Return 0 if no number is found
}

// Thread function to copy files
void* copy_file_thread(void* arg) {
    FilePair* file_pair = static_cast<FilePair*>(arg);

    std::ifstream src(file_pair->source, std::ios::binary);
    if (!src.is_open()) {
        pthread_mutex_lock(&console_mutex);
        std::cerr << "Error: Could not open source file " << file_pair->source << ": " << strerror(errno) << std::endl;
        pthread_mutex_unlock(&console_mutex);
        pthread_exit(nullptr);
    }

    std::ofstream dest(file_pair->destination, std::ios::binary);
    if (!dest.is_open()) {
        pthread_mutex_lock(&console_mutex);
        std::cerr << "Error: Could not open destination file " << file_pair->destination << ": " << strerror(errno) << std::endl;
        pthread_mutex_unlock(&console_mutex);
        src.close();
        pthread_exit(nullptr);
    }

    dest << src.rdbuf();

    pthread_mutex_lock(&console_mutex);
    std::cout << "Copied " << file_pair->source << " to " << file_pair->destination << std::endl;
    pthread_mutex_unlock(&console_mutex);

    src.close();
    dest.close();

    pthread_exit(nullptr);
}

int main(int argc, char* argv[]) {
    // Initialize the console mutex
    pthread_mutex_init(&console_mutex, nullptr);

    // Check for correct number of arguments
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <number_of_files> <source_directory> <destination_directory>" << std::endl;
        return EXIT_FAILURE;
    }

    // Parse number of files to copy
    int file_count = std::stoi(argv[1]);
    if (file_count <= 0 || file_count > 10) {
        std::cerr << "Error: number_of_files must be between 1 and 10" << std::endl;
        return EXIT_FAILURE;
    }

    std::string source_dir = argv[2];
    std::string destination_dir = argv[3];

    // Check if source directory exists and is accessible
    DIR* dir = opendir(source_dir.c_str());
    if (!dir) {
        std::cerr << "Error: Cannot open source directory " << source_dir << ": " << strerror(errno) << std::endl;
        return EXIT_FAILURE;
    }

    // Check if destination directory exists; if not, create it
    struct stat st = {0};
    if (stat(destination_dir.c_str(), &st) == -1) {
        if (mkdir(destination_dir.c_str(), 0755) != 0) {
            std::cerr << "Error: Cannot create destination directory " << destination_dir << ": " << strerror(errno) << std::endl;
            closedir(dir);
            return EXIT_FAILURE;
        } else {
            std::cout << "Destination directory " << destination_dir << " created successfully." << std::endl;
        }
    }

    // Read files from source directory
    std::vector<std::string> files;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // Skip "." and ".." entries
        if (entry->d_type == DT_REG) { // Regular file
            files.push_back(entry->d_name);
        }
    }
    closedir(dir);

    // Sort files based on numeric value extracted from filenames
    std::sort(files.begin(), files.end(), [](const std::string& a, const std::string& b) {
        return extract_number(a) < extract_number(b);
    });

    // Limit the number of files to copy
    if (files.size() > static_cast<size_t>(file_count)) {
        files.resize(file_count);
    }

    // Prepare file pairs and threads
    std::vector<FilePair> file_pairs(files.size());
    std::vector<pthread_t> threads(files.size());

    for (size_t i = 0; i < files.size(); ++i) {
        file_pairs[i].source = source_dir + "/" + files[i];
        file_pairs[i].destination = destination_dir + "/" + files[i];

        int ret = pthread_create(&threads[i], nullptr, copy_file_thread, &file_pairs[i]);
        if (ret != 0) {
            std::cerr << "Error: Failed to create thread for copying " << files[i] << ": " << strerror(ret) << std::endl;
            // Clean up already created threads
            for (size_t j = 0; j < i; ++j) {
                pthread_join(threads[j], nullptr);
            }
            return EXIT_FAILURE;
        }
    }

    // Wait for all threads to complete
    for (size_t i = 0; i < threads.size(); ++i) {
        pthread_join(threads[i], nullptr);
    }

    // Destroy the console mutex
    pthread_mutex_destroy(&console_mutex);

    std::cout << "All files have been copied successfully." << std::endl;
    return EXIT_SUCCESS;
}
