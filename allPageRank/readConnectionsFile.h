#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <vector>

/*
    Fitxer util per llegir el fitxer de conexions. Heredat del projecte personal.
*/

using IndexMap = std::unordered_map<uint32_t, std::vector<uint32_t>>;

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

        uint32_t from_idx = static_cast<uint32_t>(std::stoi(line.substr(0, sep)));
        connections[from_idx]; 

        std::istringstream iss(line.substr(sep + 4));
        std::string word;
        while (iss >> word) {
            uint32_t to_idx = static_cast<uint32_t>(std::stoi(word));
            connections[from_idx].push_back(to_idx);
            connections.try_emplace(to_idx);
        }
    }

    std::cout << "Connections: " << connections.size() << " nodes" << std::endl;
    return true;
}
