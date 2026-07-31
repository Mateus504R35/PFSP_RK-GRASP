#include "../Problem/PFSP_Problem.h"

#include <iostream>

int n = 0;
int numDecoders = 1;
int numLS = 0;

int main()
{
    char instanceName[] = "PFSP/ta001.txt";

    ReadData(instanceName);

    TSol solution;
    solution.vec.resize(n + 1);

    /*
     * Increasing keys generate the identity permutation:
     * 0, 1, 2, ..., n - 1
     */
    for (int job = 0; job < n; ++job)
    {
        solution.vec[job].rk =
            static_cast<double>(job + 1) /
            static_cast<double>(n + 1);

        solution.vec[job].sol = -1;
    }

    // Extra gene reserved for decoder selection.
    solution.vec[n].rk = 0.0;
    solution.vec[n].sol = -1;

    Dec1(solution);

    std::cout << "Decoded permutation:\n";

    for (int position = 0; position < n; ++position)
    {
        std::cout << solution.vec[position].sol;

        if (position + 1 < n)
        {
            std::cout << ' ';
        }
    }

    std::cout << "\nMakespan: " << solution.ofv << '\n';

    if (solution.ofv != 1448.0)
    {
        std::cerr
            << "Test failed: expected makespan 1448 for the identity "
            << "permutation of ta001.\n";

        return 1;
    }

    std::cout << "Test passed.\n";
    return 0;
}
