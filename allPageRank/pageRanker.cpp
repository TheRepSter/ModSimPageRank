#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <utility>
#include "readConnectionsFile.h"

using RankMap = std::unordered_map<zim::entry_index_type, double>;

void saveRanking(const RankMap &ranking, const std::string &filename) {
    std::vector<std::pair<zim::entry_index_type, double>> sorted(ranking.begin(), ranking.end());
    std::sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b) {
        return a.second > b.second;
    });

    std::ofstream file(filename);
    for (const auto &[index, rank] : sorted) {
        file << index << ": " << rank << "\n";
    }
}

void pageRank(
    const IndexMap &connections,
    RankMap &actualRanking,
    double dampingFactor,
    int maxIterations,
    double epsilon,
    const std::string &filename
) {
    double N = static_cast<double>(connections.size());
    if (N == 0) return;

    for (int i = 0; i < maxIterations; i++) {
        double dangling_sum = 0.0;
        for (const auto &[node, tos] : connections) {
            if (tos.empty()) {
                dangling_sum += actualRanking.count(node) ? actualRanking.at(node) : 0.0;
            }
        }

        double base = (1.0 - dampingFactor) / N + dampingFactor * dangling_sum / N;

        RankMap newRanking;
        newRanking.reserve(connections.size());
        for (const auto &[node, _] : connections) {
            newRanking[node] = base;
        }

        for (const auto &[from, tos] : connections) {
            if (tos.empty()) continue;
            double rank_from = actualRanking.count(from) ? actualRanking.at(from) : 0.0;
            double contribution = dampingFactor * rank_from / static_cast<double>(tos.size());
            for (const auto &to : tos) {
                newRanking[to] += contribution;
            }
        }

        // L1 convergence check
        double delta = 0.0;
        for (const auto &[node, rank] : newRanking) {
            double old_rank = actualRanking.count(node) ? actualRanking.at(node) : 0.0;
            delta += std::abs(rank - old_rank);
        }

        actualRanking = std::move(newRanking);

        if (i % 100 == 0 || delta < epsilon) {
            std::cout << "Iteration " << i << "  delta=" << delta << std::endl;
            saveRanking(actualRanking, filename);
        }

        if (delta < epsilon) {
            std::cout << "Converged at iteration " << i << " (delta=" << delta << " < epsilon=" << epsilon << ")" << std::endl;
            return;
        }
    }

    std::cout << "Reached max iterations (" << maxIterations << ") without convergence." << std::endl;
}

int main() {
    bool proper = 1;

    std::string connectionsFile = "entriesConnections.txt";
    std::string rankingFile     = "entriesRanking.txt";

    IndexMap connections;
    if (!readConnectionsFile(connectionsFile, connections)) return 1;

    double N = static_cast<double>(connections.size());
    RankMap actualRanking;

    std::ifstream actualRankingFile(rankingFile);
    if (actualRankingFile.is_open()) {
        std::string line;
        while (std::getline(actualRankingFile, line)) {
            size_t sep = line.find(": ");
            if (sep == std::string::npos) continue;
            zim::entry_index_type idx = static_cast<zim::entry_index_type>(std::stoi(line.substr(0, sep)));
            actualRanking[idx] = std::stod(line.substr(sep + 2));
        }
        actualRankingFile.close();
        std::cout << "Ranking passat trobat, continuant d'ell." << std::endl;
    } else {
        for (const auto &[node, _] : connections) {
            actualRanking[node] = 1.0 / N;
        }
        std::cout << "Ranking inicialitzat uniformement (1/N)." << std::endl;
    }

    pageRank(connections, actualRanking, 0.85, 10000, 1e-11, rankingFile);
    std::cout << "Fet!" << std::endl;
    return 0;
}
