#include "../Problem/PFSP_Problem.h"

int n = 0;
int numDecoders = 0;
int numLS = 0;

int main()
{
    char instanceName[] = "PFSP/ta001.txt";

    ReadData(instanceName);
    PrintInstance();

    return 0;
}