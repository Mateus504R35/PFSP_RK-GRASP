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
#include <numeric>
#include <sstream>
#include <string>
#include <vector>
#include <limits>

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
 Description: calculates the PFSP makespan of any valid job permutation.
              It is independent of the decoder used to create the permutation.
*************************************************************************************/
double CalculateFitness(const std::vector<int>& permutation)
{
    if (numberOfJobs <= 0 || numberOfMachines <= 0)
    {
        std::cerr << "Error: no PFSP instance has been loaded.\n";
        std::exit(EXIT_FAILURE);
    }

    if (permutation.empty() ||
        static_cast<int>(permutation.size()) > numberOfJobs)
    {
        std::cerr
            << "Error: invalid permutation size.\n";
        std::exit(EXIT_FAILURE);
    }

    std::vector<bool> visited(numberOfJobs, false);

    std::vector<long long> completionTime(
        numberOfMachines,
        0
    );

    for (int position = 0;
         position < static_cast<int>(permutation.size());
         ++position)
    {
        const int job = permutation[position];

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

        if (visited[job])
        {
            std::cerr
                << "Error: duplicated job "
                << job
                << " in the permutation.\n";

            std::exit(EXIT_FAILURE);
        }

        visited[job] = true;

        completionTime[0] +=
            processingTime[job][0];

        for (int machine = 1;
             machine < numberOfMachines;
             ++machine)
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
     * The chromosome positions represent the jobs.
     * The decoder sorts job indices according to their random keys,
     * without changing the order of the random-key genes themselves.
     */
    std::vector<int> permutation(numberOfJobs);

    std::iota(
        permutation.begin(),
        permutation.end(),
        0
    );

    std::stable_sort(
        permutation.begin(),
        permutation.end(),
        [&s](const int jobA, const int jobB)
        {
            if (s.vec[jobA].rk == s.vec[jobB].rk)
            {
                return jobA < jobB;
            }

            return s.vec[jobA].rk < s.vec[jobB].rk;
        }
    );

    /*
     * Store the decoded permutation in the sol fields so the framework
     * and future local searches can access it.
     */
    for (int position = 0; position < numberOfJobs; ++position)
    {
        s.vec[position].sol = permutation[position];
    }

    /*
     * Every decoder must evaluate its decoded permutation using the same
     * PFSP objective function.
     */
    
    s.ofv = CalculateFitness(permutation);
}


/*
 * NEH:
 *
 * - sort jobs in descending order of total processing time;
 * - build the permutation incrementally, inserting each job in the best position.
 */
void Dec2(TSol& s)
{
    if (numberOfJobs <= 0 || numberOfMachines <= 0)
    {
        std::cerr << "Error: no PFSP instance has been loaded.\n";
        std::exit(EXIT_FAILURE);
    }

    /*
     * Step 1:
     * Create the initial list of jobs.
     */
    std::vector<int> jobs(numberOfJobs);

    std::iota(
        jobs.begin(),
        jobs.end(),
        0
    );

    /*
     * Step 2:
     * Sort jobs in descending order of total processing time.
     *
     * NEH priority:
     *
     * P_j = sum of processing times of job j
     */
    std::stable_sort(
        jobs.begin(),
        jobs.end(),
        [](const int jobA, const int jobB)
        {
            long long totalA = 0;
            long long totalB = 0;

            for (int machine = 0;
                machine < numberOfMachines;
                ++machine)
            {
                totalA += processingTime[jobA][machine];
                totalB += processingTime[jobB][machine];
            }

            if (totalA == totalB)
            {
                return jobA < jobB;
            }

            return totalA > totalB;
        }
    );

    /*
     * Step 3:
     * Build the permutation incrementally.
     */
    std::vector<int> permutation;

    for (int job : jobs)
    {
        std::vector<int> bestPermutation;

        double bestFitness =
            std::numeric_limits<double>::infinity();

        /*
         * Try inserting the current job in every possible position.
         */
        for (std::size_t position = 0;
            position <= permutation.size();
            ++position)
        {
            std::vector<int> candidate = permutation;

            candidate.insert(
                candidate.begin() + position,
                job
            );

            const double candidateFitness =
                CalculateFitness(candidate);

            if (candidateFitness < bestFitness)
            {
                bestFitness = candidateFitness;
                bestPermutation = candidate;
            }
        }

        permutation = bestPermutation;
    }

    /*
     * Store the final decoded permutation in TSol.
     */
    for (int position = 0;
         position < numberOfJobs;
         ++position)
    {
        s.vec[position].sol =
            permutation[position];
    }

    /*
     * Evaluate the complete NEH solution using the same fitness
     * used by every other decoder.
     */
    s.ofv = CalculateFitness(permutation);
}

/*
 * RK + NEH hybrid decoder:
 *
 * - combines the classical NEH priority (total processing time)
 *   with the random-key priority;
 * - jobs with larger total processing time are favored, as in NEH;
 * - jobs with smaller random keys are also favored, as in Dec1;
 * - after defining the job order, the permutation is built using
 *   the standard NEH best-insertion procedure.
 */
void Dec3(TSol& s)
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
     * Weight of the NEH component in the hybrid priority.
     *
     * alpha = 1.0 -> classical NEH ordering
     * alpha = 0.0 -> pure random-key ordering
     */
    const double alpha = 0.70;

    /*
     * Step 1:
     * Calculate the total processing time of each job.
     */
    std::vector<long long> totalProcessing(numberOfJobs, 0);

    long long minTotal = std::numeric_limits<long long>::max();
    long long maxTotal = std::numeric_limits<long long>::min();

    for (int job = 0; job < numberOfJobs; ++job)
    {
        for (int machine = 0; machine < numberOfMachines; ++machine)
        {
            totalProcessing[job] += processingTime[job][machine];
        }

        minTotal = std::min(minTotal, totalProcessing[job]);
        maxTotal = std::max(maxTotal, totalProcessing[job]);
    }

    /*
     * Step 2:
     * Build one hybrid priority value for each job.
     *
     * NEH component:
     *     larger total processing time -> larger priority.
     *
     * RK component:
     *     smaller random key -> larger priority.
     *
     * Both components are kept in [0,1] before they are combined.
     */
    std::vector<double> priority(numberOfJobs, 0.0);

    for (int job = 0; job < numberOfJobs; ++job)
    {
        double nehPriority = 1.0;

        if (maxTotal != minTotal)
        {
            nehPriority =
                static_cast<double>(
                    totalProcessing[job] - minTotal
                )
                /
                static_cast<double>(
                    maxTotal - minTotal
                );
        }

        const double rkPriority = 1.0 - s.vec[job].rk;

        priority[job] =
            alpha * nehPriority
            + (1.0 - alpha) * rkPriority;
    }

    /*
     * Step 3:
     * Sort jobs according to the hybrid RK + NEH priority.
     */
    std::vector<int> jobs(numberOfJobs);

    std::iota(
        jobs.begin(),
        jobs.end(),
        0
    );

    std::stable_sort(
        jobs.begin(),
        jobs.end(),
        [&priority, &totalProcessing, &s](const int jobA, const int jobB)
        {
            if (priority[jobA] != priority[jobB])
            {
                return priority[jobA] > priority[jobB];
            }

            if (totalProcessing[jobA] != totalProcessing[jobB])
            {
                return totalProcessing[jobA] > totalProcessing[jobB];
            }

            if (s.vec[jobA].rk != s.vec[jobB].rk)
            {
                return s.vec[jobA].rk < s.vec[jobB].rk;
            }

            return jobA < jobB;
        }
    );

    /*
     * Step 4:
     * Apply the NEH best-insertion construction using the hybrid
     * order generated above.
     */
    std::vector<int> permutation;

    for (int job : jobs)
    {
        std::vector<int> bestPermutation;

        double bestFitness =
            std::numeric_limits<double>::infinity();

        for (std::size_t position = 0;
             position <= permutation.size();
             ++position)
        {
            std::vector<int> candidate = permutation;

            candidate.insert(
                candidate.begin() + position,
                job
            );

            const double candidateFitness =
                CalculateFitness(candidate);

            if (candidateFitness < bestFitness)
            {
                bestFitness = candidateFitness;
                bestPermutation = candidate;
            }
        }

        permutation = bestPermutation;
    }

    /*
     * Step 5:
     * Store the decoded permutation in TSol.
     */
    for (int position = 0;
         position < numberOfJobs;
         ++position)
    {
        s.vec[position].sol =
            permutation[position];
    }

    /*
     * All decoders use the same PFSP objective function.
     */
    s.ofv = CalculateFitness(permutation);
}

void Dec4(TSol& s) {}
void Dec5(TSol& s) {}

/************************************************************************************
 Method: Local Search Heuristics
 Description: implement local searches here if required
*************************************************************************************/
void LS1(TSol& s)
{
    // Recupera a permutacao atual produzida pelo decoder
    std::vector<int> currentPermutation(numberOfJobs);

    for (int i = 0; i < numberOfJobs; ++i)
    {
        currentPermutation[i] = s.vec[i].sol;
    }

    double currentFitness =
        CalculateFitness(currentPermutation);

    bool improved = true;

    while (improved)
    {
        improved = false;

        double bestFitness = currentFitness;
        std::vector<int> bestPermutation = currentPermutation;

        // Escolhe um job para remover
        for (int i = 0; i < numberOfJobs; ++i)
        {
            const int job = currentPermutation[i];

            // Remove o job da posicao i
            std::vector<int> partialPermutation =
                currentPermutation;

            partialPermutation.erase(
                partialPermutation.begin() + i
            );

            // Testa inserir o job em todas as posicoes
            for (int j = 0; j < numberOfJobs; ++j)
            {
                std::vector<int> candidate =
                    partialPermutation;

                candidate.insert(
                    candidate.begin() + j,
                    job
                );

                double candidateFitness =
                    CalculateFitness(candidate);

                // Best Improvement:
                // guarda o melhor movimento encontrado
                if (candidateFitness < bestFitness)
                {
                    bestFitness = candidateFitness;
                    bestPermutation = candidate;
                    improved = true;
                }
            }
        }

        // Se encontrou melhoria, move para a nova solucao
        if (improved)
        {
            currentPermutation = bestPermutation;
            currentFitness = bestFitness;
        }
    }

    // Salva a permutacao melhorada em TSol
    for (int i = 0; i < numberOfJobs; ++i)
    {
        s.vec[i].sol = currentPermutation[i];
    }

    s.ofv = currentFitness;
}

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