#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <utility>
#include "readConnectionsFile.h"

/*
    Programa que fa el PageRank iteratiu en c++. És bastant ràpid per poder fer
    el PageRank de la Viquipedia completa. Té com a input el factor de dampling.
*/

using RankMap = std::unordered_map<uint32_t, double>;

void saveRanking(const RankMap &ranking, const std::string &filename) {
    std::vector<std::pair<uint32_t, double>> sorted(ranking.begin(), ranking.end());
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
                // Recollim la suma dels nodes sense sortida per després distribuir-ho
                // a la resta de nodes de la xarxa
                dangling_sum += actualRanking.count(node) ? actualRanking.at(node) : 0.0;
            }
        }

        // Repartim la massa dels nodes sense sortida 
        double base = (1.0 - dampingFactor) / N + dampingFactor * dangling_sum / N;

        RankMap newRanking;
        newRanking.reserve(connections.size());
        for (const auto &[node, _] : connections) {
            newRanking[node] = base;
        }

        for (const auto &[from, tos] : connections) {
            if (tos.empty()) continue; // Si no te sortides no s'ha de fer res, ja està treballat.
            double rank_from = actualRanking.count(from) ? actualRanking.at(from) : 0.0;
            double contribution = dampingFactor * rank_from / static_cast<double>(tos.size());
            for (const auto &to : tos) {
                newRanking[to] += contribution;
            }
        }

        // Mirant convergencia amb L1
        double delta = 0.0;
        for (const auto &[node, rank] : newRanking) {
            double old_rank = actualRanking.count(node) ? actualRanking.at(node) : 0.0;
            delta += std::abs(rank - old_rank);
        }

        actualRanking = std::move(newRanking);

        std::cout << "Iteració " << i+1 << "  delta=" << delta << std::endl;
        if (i % 10 == 9)
            saveRanking(actualRanking, filename);

        if (delta < epsilon) {
            saveRanking(actualRanking, filename);
            std::cout << "Convergit a l'iteració " << i << " (delta=" << delta << " < epsilon=" << epsilon << ")" << std::endl;
            return;
        }
    }

    std::cout << "Ha arribat al maxim d'iteracions (" << maxIterations << ") sense convergir." << std::endl;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Us: " << argv[0] << " <factor d'esmorteïment>" << std::endl;
        return 1;
    }

    double dampingFactor = std::stod(argv[1]);
    if (dampingFactor <= 0.0 || dampingFactor >= 1.0) {
        std::cerr << "El factor d'esmorteïment ha d'estar en l'interval (0,1), exclusivament." << std::endl;
        return 1;
    }

    std::string connectionsFile = "entriesConnections.txt";
    std::string rankingFile     = "resultatsPageRank/damping" + std::to_string(static_cast<int>(dampingFactor * 100)) + ".txt";

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
            uint32_t idx = static_cast<uint32_t>(std::stoi(line.substr(0, sep)));
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

    pageRank(connections, actualRanking, dampingFactor, 10000, 1e-8, rankingFile);
    return 0;
}
