#include <sys/time.h>
#include <math.h>
#include <cstring>
#include <ctime>

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <string>
#include <algorithm>
#include <utility>
#include <numeric>
#include <map>
#include <limits>
#include <random>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>

#include "Data.h"
#include "GlobalVariables.h"
#include "GlobalFunctions.h"
#include "Output.h"
#include "../Problem/PFSP_Problem.h"
#include "../MH/Method.h"
#include "../MH/MultiStart.h"
#include "../MH/BRKGA.h"
#include "../MH/GRASP.h"

/************************************************************************************
 Scenario formats accepted

 Legacy (kept for compatibility):
 INSTANCE DEBUG DECODERS MAXTIME MAXRUNS THREADS TARGET
 PFSP/ta001.txt 1 3 10 5 1 0

 Controlled decoder comparison:
 INSTANCE DEBUG DECODERS FIXED_DECODER MAXTIME MAXRUNS THREADS TARGET
 PFSP/ta001.txt 1 3 1 10 5 1 0
 PFSP/ta001.txt 1 3 2 10 5 1 0
 PFSP/ta001.txt 1 3 3 10 5 1 0

 FIXED_DECODER = 0 keeps the original multi-decoder behavior, where the extra random
 key selects one of the DECODERS available.
************************************************************************************/
static bool ReadScenarioLine(
    std::ifstream& scenario,
    std::string& tableName,
    int& scenarioDebug,
    int& scenarioNumDecoders,
    int& scenarioFixedDecoder,
    int& scenarioMaxTime,
    int& scenarioMaxRuns,
    int& scenarioMaxThreads,
    float& scenarioOptimal)
{
    std::string line;

    while (std::getline(scenario, line))
    {
        const std::size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos || line[first] == '#')
            continue;

        // First try the new 8-column format.
        {
            std::istringstream input(line);
            std::string extra;
            if ((input >> tableName
                       >> scenarioDebug
                       >> scenarioNumDecoders
                       >> scenarioFixedDecoder
                       >> scenarioMaxTime
                       >> scenarioMaxRuns
                       >> scenarioMaxThreads
                       >> scenarioOptimal) && !(input >> extra))
            {
                return true;
            }
        }

        // Fall back to the original 7-column format.
        {
            std::istringstream input(line);
            std::string extra;
            if ((input >> tableName
                       >> scenarioDebug
                       >> scenarioNumDecoders
                       >> scenarioMaxTime
                       >> scenarioMaxRuns
                       >> scenarioMaxThreads
                       >> scenarioOptimal) && !(input >> extra))
            {
                scenarioFixedDecoder = 0;
                return true;
            }
        }

        // Non-parsable lines are treated as headers/descriptions.
    }

    return false;
}

int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        std::cerr << "Usage: " << argv[0]
                  << " <scenario_file> <method> <control>\n";
        return EXIT_FAILURE;
    }

    const std::string scenarioName = argv[1];
    const int method = std::atoi(argv[2]);
    const int control = std::atoi(argv[3]);

    if (method < 1 || method > 3)
    {
        std::cerr << "Error: invalid method " << method
                  << ". Use 1 (MultiStart), 2 (BRKGA), or 3 (GRASP).\n";
        return EXIT_FAILURE;
    }

    std::ifstream scenario(scenarioName);
    if (!scenario.is_open())
    {
        std::cerr << "Error: could not open scenario file: "
                  << scenarioName << '\n';
        return EXIT_FAILURE;
    }

    TSol sBest;
    sBest.flag = 0;
    sBest.label = 0;
    sBest.ofv = std::numeric_limits<double>::infinity();
    sBest.similar = 0;
    sBest.promising = 0;
    sBest.vec.clear();

    std::string tableName;
    int scenarioDebug = 0;
    int scenarioNumDecoders = 1;
    int scenarioFixedDecoder = 0;
    int scenarioMaxTime = 0;
    int scenarioMaxRuns = 1;
    int scenarioMaxThreads = 1;
    float scenarioOptimal = 0.0f;

    while (ReadScenarioLine(
        scenario,
        tableName,
        scenarioDebug,
        scenarioNumDecoders,
        scenarioFixedDecoder,
        scenarioMaxTime,
        scenarioMaxRuns,
        scenarioMaxThreads,
        scenarioOptimal))
    {
        debug = scenarioDebug;
        numDecoders = scenarioNumDecoders;
        fixedDecoder = scenarioFixedDecoder;
        MAXTIME = scenarioMaxTime;
        MAXRUNS = scenarioMaxRuns;
        MAX_THREADS = scenarioMaxThreads;
        OPTIMAL = scenarioOptimal;

        if (numDecoders < 1 || numDecoders > 3)
        {
            std::cerr << "Error: DECODERS must be between 1 and 3 for PFSP.\n";
            return EXIT_FAILURE;
        }

        if (fixedDecoder < 0 || fixedDecoder > numDecoders)
        {
            std::cerr << "Error: FIXED_DECODER must be 0 (multi) or between 1 and "
                      << numDecoders << ".\n";
            return EXIT_FAILURE;
        }

        if (MAXTIME <= 0 || MAXRUNS <= 0 || MAX_THREADS <= 0)
        {
            std::cerr << "Error: MAXTIME, MAXRUNS and THREADS must be greater than zero.\n";
            return EXIT_FAILURE;
        }

        std::strncpy(nameTable, tableName.c_str(), sizeof(nameTable) - 1);
        nameTable[sizeof(nameTable) - 1] = '\0';
        std::strncpy(instance, nameTable, sizeof(instance) - 1);
        instance[sizeof(instance) - 1] = '\0';

        double foBest = std::numeric_limits<double>::infinity();
        double foAverage = 0.0;
        float timeBest = 0.0f;
        float timeTotal = 0.0f;
        std::vector<double> ofvs;
        ofvs.reserve(MAXRUNS);

        sBest.ofv = std::numeric_limits<double>::infinity();
        sBest.vec.clear();

        std::cout << "\n\nInstance: " << instance
                  << " [Threads " << MAX_THREADS << "]\n";
        std::cout << "Available decoders: " << numDecoders << "\n";
        if (fixedDecoder == 0)
            std::cout << "Decoder mode: MULTI (selected by the extra RK gene)\n";
        else
            std::cout << "Decoder mode: FIXED Dec" << fixedDecoder << "\n";
        std::cout << "Target: " << OPTIMAL << "\n";
        std::cout << "Run: ";

        for (int run = 0; run < MAXRUNS; ++run)
        {
            // Same deterministic seeds are used by Dec1/Dec2/Dec3 when DEBUG=1.
            unsigned int seed = 0;
            if (debug == 1)
            {
                seed = static_cast<unsigned int>(run + 1);
            }
            else
            {
                seed = static_cast<unsigned int>(
                    std::chrono::high_resolution_clock::now()
                        .time_since_epoch().count())
                    ^ static_cast<unsigned int>(run + 1);
            }
            SetSeed(seed);

            std::cout << run + 1 << ' ';

            gettimeofday(&Tstart, NULL);
            gettimeofday(&Tend, NULL);
            gettimeofday(&Tbest, NULL);

            bestSolution.ofv = std::numeric_limits<double>::infinity();
            bestSolution.vec.clear();

            FreeMemoryProblem();
            ReadData(nameTable);

            if (method > 1)
                CretePoolSolutions();

            switch (method)
            {
                case 1:
                    std::strcpy(nameMH, "MultiStart");
                    MultiStart();
                    break;
                case 2:
                    std::strcpy(nameMH, "BRKGA");
                    BRKGA(method, control);
                    break;
                case 3:
                    std::strcpy(nameMH, "GRASP");
                    GRASP(method, control);
                    break;
            }

            gettimeofday(&Tend, NULL);

            if (bestSolution.ofv < sBest.ofv)
                sBest = bestSolution;
            if (bestSolution.ofv < foBest)
                foBest = bestSolution.ofv;

            foAverage += bestSolution.ofv;
            ofvs.push_back(bestSolution.ofv);

            timeBest += ((Tbest.tv_sec - Tstart.tv_sec) * 1000000u
                         + Tbest.tv_usec - Tstart.tv_usec) / 1.e6;
            timeTotal += ((Tend.tv_sec - Tstart.tv_sec) * 1000000u
                          + Tend.tv_usec - Tstart.tv_usec) / 1.e6;
        }

        foAverage /= MAXRUNS;
        timeBest /= MAXRUNS;
        timeTotal /= MAXRUNS;
        std::cout << '\n';

        // Distinguish controlled decoder experiments in result file names.
        char outputMethod[256];
        if (fixedDecoder == 0)
            std::snprintf(outputMethod, sizeof(outputMethod), "%s_MultiDec", nameMH);
        else
            std::snprintf(outputMethod, sizeof(outputMethod), "%s_Dec%d", nameMH, fixedDecoder);

        if (!debug)
        {
            WriteSolution(outputMethod, sBest, n, timeBest, timeTotal, instance);
            WriteResults(outputMethod, foBest, foAverage, ofvs,
                         timeBest, timeTotal, instance);
        }
        else
        {
            WriteSolutionScreen(outputMethod, sBest, n,
                                timeBest, timeTotal, instance);
        }

        FreeMemoryProblem();
    }

    return EXIT_SUCCESS;
}
