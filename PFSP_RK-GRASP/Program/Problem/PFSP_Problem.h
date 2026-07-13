// *******************************************************************
//      file with specific functions to solve a Problem
// *******************************************************************
#ifndef _PROBLEM_H
#define _PROBLEM_H

#include "../Main/Data.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>


// Variables declared in main.cpp
extern int numDecoders;                     // number of decoders
extern int numLS;                           // 0 - without local search     > k - number of local search heuristics
extern int n;                               // size of cromossoms

//----------------- DEFINITION OF TYPES OF PROBLEM SPECIFIC -----------------------


//------ DEFINITION OF GLOBAL CONSTANTS AND VARIABLES OF SPECIFIC PROBLEM  --------
int numberOfJobs;
int numberOfMachines;

std::vector<std::vector<int>> processingTime;


//-------------------------- FUNCTIONS OF SPECIFIC PROBLEM --------------------------

/************************************************************************************
 Method: ReadNextDataLine
 Description: ignores empty lines, leading spaces and tabs, and lines where the first relevant character is #
*************************************************************************************/
bool ReadNextDataLine(std::ifstream &file,std::string &line)
{
    while (std::getline(file,line))
    {
        std::size_t firstCharacter = line.find_first_not_of(" \t\r\n");

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
    std::cout
        << "Jobs: "
        << numberOfJobs
        << '\n';

    std::cout
        << "Machines: "
        << numberOfMachines
        << '\n';

    for (int job = 0; job < numberOfJobs; job++)
    {
        std::cout << "Job " << job << ": ";

        for (int machine = 0; machine < numberOfMachines; machine++)
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
 Description: read the input data
*************************************************************************************/
void ReadData(char nameTable[])
{
    std::string filePath = "../Instances/";
    filePath += nameTable;

    std::ifstream file(filePath);

    if (!file.is_open())
    {
        std::cerr
            << "Error: could not open the instance: "
            << filePath
            << '\n';

        std::exit(EXIT_FAILURE);
    }

    std::string line;

    /*
     * First number line:
     *
     * number_of_jobs number_of_machines
     */
    if (!ReadNextDataLine(file,line))
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
     * The Taillard file stores:
     *
     * times[machine][task]
     *
     * Internally:
     *
     * processingTime[job][machine]
     */
    processingTime.assign(
        numberOfJobs,
        std::vector<int>(numberOfMachines,0)
    );

    for (int machine = 0; machine < numberOfMachines; machine++)
    {
        if (!ReadNextDataLine(file,line))
        {
            std::cerr
                << "Error: processing times are missing "
                << "for the machine "
                << machine
                << ".\n";

            std::exit(EXIT_FAILURE);
        }

        std::istringstream processingTimes(line);

        for (int job = 0; job < numberOfJobs; job++)
        {
            int time;

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
                    << "Error: negative processing time "
                    << "for task "
                    << job
                    << " on machine "
                    << machine
                    << ".\n";

                std::exit(EXIT_FAILURE);
            }

            processingTime[job][machine] = time;
        }

        int extraValue;

        if (processingTimes >> extraValue)
        {
            std::cerr
                << "Error: Machine line "
                << machine
                << " has more than "
                << numberOfJobs
                << " timings.\n";

            std::exit(EXIT_FAILURE);
        }
    }

    file.close();

    /*
     * Each random key represents a job.
     */
    n = numberOfJobs;

    std::cout
        << "Instance loaded: "
        << numberOfJobs
        << " jobs and "
        << numberOfMachines
        << " machines.\n";

    PrintInstance();
}

/************************************************************************************
 Method: CalculateFitness
 Description: calculate the fitness of a chromossom s
*************************************************************************************/
double CalculateFitness(TSol s)
{    
    s.ofv = 0;

    return s.ofv;
}

/************************************************************************************
 Method: Decoders
 Description: users need to implement at least one decoder, DecK (K = [1,2,3,4,5])
*************************************************************************************/

void Dec1(TSol &s) 
{
    // create a solution of the problem

    s.ofv = CalculateFitness(s);
}

void Dec2(TSol &s){}
void Dec3(TSol &s){}
void Dec4(TSol &s){}
void Dec5(TSol &s){}

/************************************************************************************
 Method: Local Search Heuristics
 Description: users need to implement local searchs heuristics if he/she use the RVND,
 LSK (K = [1,2,3,4,5])
*************************************************************************************/

void LS1(TSol &s){}
void LS2(TSol &s){}
void LS3(TSol &s){}
void LS4(TSol &s){}
void LS5(TSol &s){}

/************************************************************************************
 Method: FreeMemoryProblem
 Description: Free local memory allocate by Problem
*************************************************************************************/
void FreeMemoryProblem(){}

#endif