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
 Method: ReadScenarioLine
 Description:
    Reads one valid PFSP scenario line.

 Expected format:
    instance debug numDecoders MAXTIME MAXRUNS MAX_THREADS OPTIMAL

 Example:
    PFSP/ta001.txt 1 3 10 1 1 0

 Empty lines, comments beginning with '#', and header lines are ignored.
*************************************************************************************/
bool ReadScenarioLine(
    std::ifstream& scenario,
    std::string& tableName,
    int& scenarioDebug,
    int& scenarioNumDecoders,
    int& scenarioMaxTime,
    int& scenarioMaxRuns,
    int& scenarioMaxThreads,
    float& scenarioOptimal
)
{
    std::string line;

    while (std::getline(scenario, line))
    {
        const std::size_t firstCharacter =
            line.find_first_not_of(" \t\r\n");

        if (firstCharacter == std::string::npos)
        {
            continue;
        }

        if (line[firstCharacter] == '#')
        {
            continue;
        }

        std::istringstream input(line);

        if (input
            >> tableName
            >> scenarioDebug
            >> scenarioNumDecoders
            >> scenarioMaxTime
            >> scenarioMaxRuns
            >> scenarioMaxThreads
            >> scenarioOptimal)
        {
            return true;
        }

        /*
         * If the line cannot be parsed, treat it as a header or
         * descriptive line and continue reading.
         */
    }

    return false;
}


/************************************************************************************
 Method: main
 Description:
    Executes the Random-Key Optimization framework for PFSP instances.
*************************************************************************************/
int main(int argc, char* argv[])
{
    /*
     * Expected command:
     *
     * ./program scenario.txt method control
     *
     * method:
     *     1 = MultiStart
     *     2 = BRKGA
     *     3 = GRASP
     */
    if (argc < 4)
    {
        std::cerr
            << "Usage: "
            << argv[0]
            << " <scenario_file> <method> <control>\n";

        return EXIT_FAILURE;
    }

    const std::string scenarioName = argv[1];
    const int method = std::atoi(argv[2]);
    const int control = std::atoi(argv[3]);

    if (method < 1 || method > 3)
    {
        std::cerr
            << "Error: invalid method "
            << method
            << ". Use 1 (MultiStart), 2 (BRKGA), or 3 (GRASP).\n";

        return EXIT_FAILURE;
    }

    std::ifstream scenario(scenarioName);

    if (!scenario.is_open())
    {
        std::cerr
            << "Error: could not open scenario file: "
            << scenarioName
            << '\n';

        return EXIT_FAILURE;
    }

    /*
     * Best solution among all runs of the current instance.
     */
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
    int scenarioMaxTime = 0;
    int scenarioMaxRuns = 1;
    int scenarioMaxThreads = 1;
    float scenarioOptimal = 0.0f;

    /*
     * Execute every valid instance listed in the scenario file.
     */
    while (ReadScenarioLine(
        scenario,
        tableName,
        scenarioDebug,
        scenarioNumDecoders,
        scenarioMaxTime,
        scenarioMaxRuns,
        scenarioMaxThreads,
        scenarioOptimal))
    {
        /*
         * Copy scenario values to the global variables expected
         * by the original RKO framework.
         */
        debug = scenarioDebug;
        numDecoders = scenarioNumDecoders;
        MAXTIME = scenarioMaxTime;
        MAXRUNS = scenarioMaxRuns;
        MAX_THREADS = scenarioMaxThreads;
        OPTIMAL = scenarioOptimal;

        if (numDecoders < 1 || numDecoders > 5)
        {
            std::cerr
                << "Error: invalid number of decoders for instance "
                << tableName
                << ": "
                << numDecoders
                << ". Expected a value between 1 and 5.\n";

            return EXIT_FAILURE;
        }

        if (MAXRUNS <= 0)
        {
            std::cerr
                << "Error: MAXRUNS must be greater than zero.\n";

            return EXIT_FAILURE;
        }

        if (MAX_THREADS <= 0)
        {
            std::cerr
                << "Error: MAX_THREADS must be greater than zero.\n";

            return EXIT_FAILURE;
        }

        /*
         * PFSP_Problem::ReadData() expects a C string containing
         * the path relative to ../Instances/.
         */
        std::strncpy(
            nameTable,
            tableName.c_str(),
            sizeof(nameTable) - 1
        );

        nameTable[sizeof(nameTable) - 1] = '\0';

        std::strncpy(
            instance,
            nameTable,
            sizeof(instance) - 1
        );

        instance[sizeof(instance) - 1] = '\0';

        double foBest = std::numeric_limits<double>::infinity();
        double foAverage = 0.0;

        float timeBest = 0.0f;
        float timeTotal = 0.0f;

        std::vector<double> ofvs;
        ofvs.reserve(MAXRUNS);

        sBest.ofv = std::numeric_limits<double>::infinity();
        sBest.vec.clear();

        std::cout
            << "\n\nInstance: "
            << instance
            << " [Threads "
            << MAX_THREADS
            << "]\n";

        std::cout
            << "Decoders: "
            << numDecoders
            << "\n";

        std::cout << "Run: ";

        for (int run = 0; run < MAXRUNS; ++run)
        {
            /*
             * The original framework uses rand()/srand().
             *
             * In debug mode, deterministic seeds are preserved.
             * Otherwise, mix current time with the run number so
             * consecutive runs do not accidentally receive the same seed.
             */
            unsigned int seed = 0;

            if (debug == 1)
            {
                seed = static_cast<unsigned int>(run + 1);
            }
            else
            {
                seed =
                    static_cast<unsigned int>(
                        std::chrono::high_resolution_clock::now()
                            .time_since_epoch()
                            .count()
                    )
                    ^ static_cast<unsigned int>(run + 1);
            }

            std::srand(seed);

            std::cout << run + 1 << ' ';

            gettimeofday(&Tstart, NULL);
            gettimeofday(&Tend, NULL);
            gettimeofday(&Tbest, NULL);

            /*
             * Best solution found in this run.
             */
            bestSolution.ofv =
                std::numeric_limits<double>::infinity();

            bestSolution.vec.clear();

            /*
             * Remove data from the previous run/instance.
             *
             * FreeMemoryProblem() sets n = 0.
             */
            FreeMemoryProblem();

            /*
             * Load the PFSP instance.
             *
             * ReadData() sets:
             *     numberOfJobs
             *     numberOfMachines
             *     processingTime
             *     n = numberOfJobs
             */
            ReadData(nameTable);

            /*
             * The framework reserves one extra gene for decoder selection.
             * The solution creation routines therefore use the new PFSP
             * value of n established by ReadData().
             */
            if (method > 1)
            {
                CretePoolSolutions();
            }

            /*
             * Execute the chosen Random-Key Optimization method.
             */
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

            /*
             * Store the best solution among all runs.
             * PFSP is a minimization problem, so smaller makespan is better.
             */
            if (bestSolution.ofv < sBest.ofv)
            {
                sBest = bestSolution;
            }

            if (bestSolution.ofv < foBest)
            {
                foBest = bestSolution.ofv;
            }

            foAverage += bestSolution.ofv;
            ofvs.push_back(bestSolution.ofv);

            timeBest +=
                ((Tbest.tv_sec - Tstart.tv_sec) * 1000000u
                    + Tbest.tv_usec
                    - Tstart.tv_usec)
                / 1.e6;

            timeTotal +=
                ((Tend.tv_sec - Tstart.tv_sec) * 1000000u
                    + Tend.tv_usec
                    - Tstart.tv_usec)
                / 1.e6;
        }

        foAverage /= MAXRUNS;
        timeBest /= MAXRUNS;
        timeTotal /= MAXRUNS;

        std::cout << '\n';

        /*
         * Keep the original output behavior of the RKO framework.
         */
        if (!debug)
        {
            WriteSolution(
                nameMH,
                sBest,
                n,
                timeBest,
                timeTotal,
                instance
            );

            WriteResults(
                nameMH,
                foBest,
                foAverage,
                ofvs,
                timeBest,
                timeTotal,
                instance
            );
        }
        else
        {
            WriteSolutionScreen(
                nameMH,
                sBest,
                n,
                timeBest,
                timeTotal,
                instance
            );
        }

        /*
         * Free the PFSP data before proceeding to the next instance.
         */
        FreeMemoryProblem();
    }

    scenario.close();

    return EXIT_SUCCESS;
}
