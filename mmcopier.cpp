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

// Mutex to protect shared resources (if needed)
pthread_mutex_t mutex;

// Helper function to extract the numeric part of the filename for sorting
int extract_number(const fs::path& path) {
    std::string filename = path.filename().string();
    std::regex number_regex("(\\d+)");
    std::smatch match;
    if (std::regex_search(filename, match, number_regex)) {
        return std::stoi(match.str(1));
    }
    return 0;
}

void* copy_file(void* args) {
    // Extract source and destination paths from arguments
    std::pair<fs::path, fs::path>* paths = static_cast<std::pair<fs::path, fs::path>*>(args);
    const fs::path& source = paths->first;
    const fs::path& destination = paths->second;

    try {
        fs::copy_file(source, destination, fs::copy_options::overwrite_existing);
        std::cout << "Copied " << source.filename() << " to " << destination << std::endl;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error copying " << source << " to " << destination << ": " << e.what() << std::endl;
    }

    return nullptr;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <n> <source_dir> <destination_dir>" << std::endl;
        return 1;
    }

    int n = std::stoi(argv[1]);
    if (n > 10) {
        std::cerr << "Error: n should be less than or equal to 10" << std::endl;
        return 1;
    }

    fs::path source_dir = argv[2];
    fs::path destination_dir = argv[3];

    if (!fs::exists(source_dir) || !fs::is_directory(source_dir)) {
        std::cerr << "Error: Source directory does not exist or is not a directory." << std::endl;
        return 1;
    }

    if (!fs::exists(destination_dir)) {
        std::cerr << "Destination directory does not exist. Creating it..." << std::endl;
        fs::create_directory(destination_dir);
    }

    // Initialize the mutex
    pthread_mutex_init(&mutex, nullptr);

    // Get all file paths in the source directory and sort them numerically
    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(source_dir)) {
        if (fs::is_regular_file(entry.path())) {
            files.push_back(entry.path());
        }
    }

    // Sort files numerically based on extracted numbers from the filename
    std::sort(files.begin(), files.end(), [](const fs::path& a, const fs::path& b) {
        return extract_number(a) < extract_number(b);
    });

    // Limit to n files
    if (files.size() > static_cast<size_t>(n)) {
        files.resize(n);
    }

    std::vector<pthread_t> threads(files.size());
    std::vector<std::pair<fs::path, fs::path>> paths(files.size());

    for (size_t i = 0; i < files.size(); ++i) {
        fs::path source_file = files[i];
        fs::path destination_file = destination_dir / source_file.filename();
        paths[i] = std::make_pair(source_file, destination_file);

        int result = pthread_create(&threads[i], nullptr, copy_file, &paths[i]);
        if (result != 0) {
            std::cerr << "Error creating thread " << i << ": " << std::strerror(result) << std::endl;
            return 1;
        }
    }

    // Wait for all threads to complete
    for (size_t i = 0; i < files.size(); i++) {
        pthread_join(threads[i], nullptr);
    }

    // Destroy the mutex
    pthread_mutex_destroy(&mutex);

    std::cout << "All files copied successfully." << std::endl;
    return 0;
}
