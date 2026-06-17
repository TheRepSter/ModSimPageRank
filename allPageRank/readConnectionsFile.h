#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <zim/archive.h>

using IndexMap = std::unordered_map<zim::entry_index_type, std::vector<zim::entry_index_type>>;

// Reads a connections file with the format:
//   from_idx :: to_idx1 to_idx2 ...
// Each node (both source and destination) is guaranteed to exist as a key in the map.
// Returns false if the file could not be opened.
inline bool readConnectionsFile(const std::string &filename, IndexMap &connections) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: no s'ha pogut obrir " << filename << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t sep = line.find(" :: ");
        if (sep == std::string::npos) continue;

        zim::entry_index_type from_idx = static_cast<zim::entry_index_type>(std::stoi(line.substr(0, sep)));
        connections[from_idx]; // ensure node exists even with no outgoing links

        std::istringstream iss(line.substr(sep + 4));
        std::string word;
        while (iss >> word) {
            zim::entry_index_type to_idx = static_cast<zim::entry_index_type>(std::stoi(word));
            connections[from_idx].push_back(to_idx);
            connections.try_emplace(to_idx); // ensure destination node exists
        }
    }

    std::cout << "Connections loaded: " << connections.size() << " nodes" << std::endl;
    return true;
}
