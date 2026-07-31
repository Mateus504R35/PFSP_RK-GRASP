// *******************************************************************
//      file with specific functions to solve a Problem
// *******************************************************************
#ifndef _PROBLEM_H
#define _PROBLEM_H

#include "../Main/Data.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Variables declared in main.cpp
extern int numDecoders; // number of decoders
extern int numLS;       // 0 - without local search; > k - number of local searches
extern int n;           // size of chromosomes

//----------------- DEFINITION OF TYPES OF PROBLEM SPECIFIC -----------------------

//------ DEFINITION OF GLOBAL CONSTANTS AND VARIABLES OF SPECIFIC PROBLEM ---------
int numberOfJobs = 0;
int numberOfMachines = 0;

std::vector<std::vector<int> > processingTime;

//-------------------------- FUNCTIONS OF SPECIFIC PROBLEM --------------------------

/************************************************************************************
 Method: ReadNextDataLine
 Description: ignores empty lines, leading spaces/tabs and comment lines
*************************************************************************************/
bool ReadNextDataLine(std::ifstream& file, std::string& line)
{
    while (std::getline(file, line))
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

        line = line.substr(firstCharacter);
        return true;
    }

    return false;
}

/************************************************************************************
 Method: PrintInstance
 Description: prints and validates the loaded instance
*************************************************************************************/
void PrintInstance()
{
    std::cout << "Jobs: " << numberOfJobs << '\n';
    std::cout << "Machines: " << numberOfMachines << '\n';

    for (int job = 0; job < numberOfJobs; ++job)
    {
        std::cout << "Job " << job << ": ";

        for (int machine = 0; machine < numberOfMachines; ++machine)
        {
            std::cout << processingTime[job][machine];

            if (machine + 1 < numberOfMachines)
            {
                std::cout << ' ';
            }
        }

        std::cout << '\n';
    }
}

/************************************************************************************
 Method: ReadData
 Description: reads a Taillard PFSP instance
*************************************************************************************/
void ReadData(char nameTable[])
{
    std::string filePath = "../Instances/";
    filePath += nameTable;

    std::ifstream file(filePath.c_str());

    if (!file.is_open())
    {
        std::cerr
            << "Error: could not open the instance: "
            << filePath
            << '\n';

        std::exit(EXIT_FAILURE);
    }

    std::string line;

    // First relevant line: number_of_jobs number_of_machines
    if (!ReadNextDataLine(file, line))
    {
        std::cerr
            << "Error: the instance does not have the row "
            << "with the number of jobs and machines.\n";

        std::exit(EXIT_FAILURE);
    }

    {
        std::istringstream dimensions(line);

        if (!(dimensions >> numberOfJobs >> numberOfMachines))
        {
            std::cerr
                << "Error: invalid format in the dimensions line: "
                << line
                << '\n';

            std::exit(EXIT_FAILURE);
        }
    }

    if (numberOfJobs <= 0 || numberOfMachines <= 0)
    {
        std::cerr
            << "Error: the number of jobs and machines must be positive.\n";

        std::exit(EXIT_FAILURE);
    }

    /*
     * Taillard file:
     *     times[machine][job]
     *
     * Internal representation:
     *     processingTime[job][machine]
     */
    processingTime.assign(
        numberOfJobs,
        std::vector<int>(numberOfMachines, 0)
    );

    for (int machine = 0; machine < numberOfMachines; ++machine)
    {
        if (!ReadNextDataLine(file, line))
        {
            std::cerr
                << "Error: processing times are missing for machine "
                << machine
                << ".\n";

            std::exit(EXIT_FAILURE);
        }

        std::istringstream processingTimes(line);

        for (int job = 0; job < numberOfJobs; ++job)
        {
            int time = 0;

            if (!(processingTimes >> time))
            {
                std::cerr
                    << "Error: machine "
                    << machine
                    << " does not have "
                    << numberOfJobs
                    << " processing times.\n";

                std::exit(EXIT_FAILURE);
            }

            if (time < 0)
            {
                std::cerr
                    << "Error: negative processing time for job "
                    << job
                    << " on machine "
                    << machine
                    << ".\n";

                std::exit(EXIT_FAILURE);
            }

            processingTime[job][machine] = time;
        }

        int extraValue = 0;

        if (processingTimes >> extraValue)
        {
            std::cerr
                << "Error: machine line "
                << machine
                << " has more than "
                << numberOfJobs
                << " processing times.\n";

            std::exit(EXIT_FAILURE);
        }
    }

    file.close();

    // Each random key represents one job.
    n = numberOfJobs;

    std::cout
        << "Instance loaded: "
        << numberOfJobs
        << " jobs and "
        << numberOfMachines
        << " machines.\n";

    // Enable only when debugging the input:
    // PrintInstance();
}

/************************************************************************************
 Method: CalculateFitness
 Description: calculates the PFSP makespan of the decoded permutation
*************************************************************************************/
double CalculateFitness(const TSol& s)
{
    if (numberOfJobs <= 0 || numberOfMachines <= 0)
    {
        std::cerr << "Error: no PFSP instance has been loaded.\n";
        std::exit(EXIT_FAILURE);
    }

    if (static_cast<int>(s.vec.size()) < numberOfJobs)
    {
        std::cerr
            << "Error: solution vector has fewer positions than jobs.\n";
        std::exit(EXIT_FAILURE);
    }

    /*
     * completionTime[m] is the completion time on machine m after
     * processing the jobs already visited in the permutation.
     */
    std::vector<long long> completionTime(numberOfMachines, 0);

    for (int position = 0; position < numberOfJobs; ++position)
    {
        const int job = s.vec[position].sol;

        if (job < 0 || job >= numberOfJobs)
        {
            std::cerr
                << "Error: invalid job "
                << job
                << " at permutation position "
                << position
                << ".\n";

            std::exit(EXIT_FAILURE);
        }

        // First machine.
        completionTime[0] += processingTime[job][0];

        // Remaining machines.
        for (int machine = 1; machine < numberOfMachines; ++machine)
        {
            completionTime[machine] =
                std::max(
                    completionTime[machine],
                    completionTime[machine - 1]
                )
                + processingTime[job][machine];
        }
    }

    return static_cast<double>(
        completionTime[numberOfMachines - 1]
        );
}

/************************************************************************************
 Method: Decoders
 Description: users need to implement at least one decoder, DecK (K = [1,2,3,4,5])
*************************************************************************************/

/*
 * Random-key sorting decoder:
 *
 * - random-key position i represents job i;
 * - jobs are sorted by increasing key;
 * - the resulting permutation is evaluated by its makespan.
 */
void Dec1(TSol& s)
{
    if (static_cast<int>(s.vec.size()) < numberOfJobs)
    {
        std::cerr
            << "Error: solution vector has fewer positions than jobs.\n";
        std::exit(EXIT_FAILURE);
    }

    /*
     * Reassociate each random-key position with its original job.
     * This is required because the framework keeps the decoded values
     * in the sol fields between calls to Decoder().
     */
    for (int job = 0; job < numberOfJobs; ++job)
    {
        s.vec[job].sol = job;
    }

    /*
     * Sort only the job genes. The extra position s.vec[n] is reserved
     * by RK-GRASP for decoder selection.
     */
    std::stable_sort(
        s.vec.begin(),
        s.vec.begin() + numberOfJobs,
        [](const TVecSol& a, const TVecSol& b)
        {
            if (a.rk == b.rk)
            {
                return a.sol < b.sol;
            }

            return a.rk < b.rk;
        }
    );

    s.ofv = CalculateFitness(s);
}

void Dec2(TSol& s) {}
void Dec3(TSol& s) {}
void Dec4(TSol& s) {}
void Dec5(TSol& s) {}

/************************************************************************************
 Method: Local Search Heuristics
 Description: implement local searches here if required
*************************************************************************************/
void LS1(TSol& s) {}
void LS2(TSol& s) {}
void LS3(TSol& s) {}
void LS4(TSol& s) {}
void LS5(TSol& s) {}

/************************************************************************************
 Method: FreeMemoryProblem
 Description: frees memory allocated by the problem
*************************************************************************************/
void FreeMemoryProblem()
{
    processingTime.clear();
    numberOfJobs = 0;
    numberOfMachines = 0;
    n = 0;
}

#endif
