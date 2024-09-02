#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <filesystem>
#include <pthread.h>
#include <cstring>
#include <algorithm>
#include <regex>

namespace fs = std::filesystem;

// Mutex to protect shared resources, ensuring thread safety
pthread_mutex_t file_copy_mutex;

// Helper function to extract numeric values from filenames for sorting
// This function uses regular expressions to find the first sequence of digits in the filename
int get_numeric_part(const fs::path& file_path) {
    std::string filename = file_path.filename().string();
    std::regex number_regex("(\\d+)"); // Regex to match one or more digits
    std::smatch match;
    if (std::regex_search(filename, match, number_regex)) {
        return std::stoi(match.str(1)); // Convert the first matched group to an integer
    }
    return 0; // Return 0 if no numeric part is found
}

// Function executed by each thread to copy a file
// This function takes a pair of paths as arguments: the source file and the destination file
void* copy_file_thread(void* args) {
    // Extract the source and destination paths from the arguments
    auto* file_paths = static_cast<std::pair<fs::path, fs::path>*>(args);
    const fs::path& source_file = file_paths->first;
    const fs::path& destination_file = file_paths->second;

    try {
        // Perform the file copy operation, overwriting the destination file if it exists
        fs::copy_file(source_file, destination_file, fs::copy_options::overwrite_existing);
        std::cout << "Copied " << source_file.filename() << " to " << destination_file << std::endl;
    } catch (const fs::filesystem_error& e) {
        // Handle any errors that occur during the file copy operation
        std::cerr << "Failed to copy " << source_file << " to " << destination_file << ": " << e.what() << std::endl;
    }

    return nullptr;
}

int main(int argc, char* argv[]) {
    // Ensure the correct number of arguments are provided
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <number_of_files> <source_directory> <destination_directory>" << std::endl;
        return 1;
    }

    // Parse the number of files to copy from the command line arguments
    int file_count = std::stoi(argv[1]);
    if (file_count > 10) {
        std::cerr << "Error: The number of files to copy should not exceed 10." << std::endl;
        return 1;
    }

    fs::path source_directory = argv[2];
    fs::path destination_directory = argv[3];

    // Verify that the source directory exists and is a valid directory
    if (!fs::exists(source_directory) || !fs::is_directory(source_directory)) {
        std::cerr << "Error: Source directory does not exist or is not a directory." << std::endl;
        return 1;
    }

    // If the destination directory does not exist, create it
    if (!fs::exists(destination_directory)) {
        std::cerr << "Destination directory does not exist. Creating it..." << std::endl;
        fs::create_directory(destination_directory);
    }

    // Initialize the mutex to synchronize access to shared resources
    pthread_mutex_init(&file_copy_mutex, nullptr);

    // Gather all file paths from the source directory
    std::vector<fs::path> source_files;
    for (const auto& entry : fs::directory_iterator(source_directory)) {
        if (fs::is_regular_file(entry.path())) {
            source_files.push_back(entry.path());
        }
    }

    // Sort the files numerically based on numbers extracted from filenames
    std::sort(source_files.begin(), source_files.end(), [](const fs::path& a, const fs::path& b) {
        return get_numeric_part(a) < get_numeric_part(b);
    });

    // Limit the number of files to copy to the specified number
    if (source_files.size() > static_cast<size_t>(file_count)) {
        source_files.resize(file_count);
    }

    // Create threads to copy each file in parallel
    std::vector<pthread_t> copy_threads(source_files.size());
    std::vector<std::pair<fs::path, fs::path>> file_paths(source_files.size());

    for (size_t i = 0; i < source_files.size(); ++i) {
        fs::path source_file = source_files[i];
        fs::path destination_file = destination_directory / source_file.filename();
        file_paths[i] = std::make_pair(source_file, destination_file);

        // Create a new thread for each file copy operation
        int result = pthread_create(&copy_threads[i], nullptr, copy_file_thread, &file_paths[i]);
        if (result != 0) {
            std::cerr << "Failed to create thread " << i << ": " << std::strerror(result) << std::endl;
            return 1;
        }
    }

    // Wait for all threads to complete their file copy operations
    for (size_t i = 0; i < source_files.size(); i++) {
        pthread_join(copy_threads[i], nullptr);
    }

    // Destroy the mutex after all threads have finished
    pthread_mutex_destroy(&file_copy_mutex);

    std::cout << "All files have been copied successfully." << std::endl;
    return 0;
}
