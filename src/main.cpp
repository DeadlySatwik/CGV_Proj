#include "simulator/Simulator.h"
#include <iostream>
using namespace std;

int main(int argc, char** argv)
{
    EngineCore::SetCmdArgs(argc, argv);

    cout << "      Project for CGV    " << endl;
    cout << "      City traffic simulation" << endl;
    cout << "      Copyright (C) DeadlyS 2026" << endl;
    cout << endl << endl;
    cout << "   Steering: " << endl << endl;
    cout << " W,A,S,D       - movement" << endl;
    cout << " Q, E          - vertical movement" << endl;
    cout << endl;
    cout << " T, Y          - decrease/increase updates per frame" << endl;
    cout << " G, H          - decrease/increase time scale" << endl;
    cout << endl;
    cout << " dragging cursor - rotating camera" << endl;
    cout << endl;
    cout << " ESC           - exit" << endl << endl << endl;

    try
    {
        Simulator *simulator = &Simulator::getInstance();

        simulator->loadRoad("exampleRoad.txt");
        simulator->loadRightOfWay("exampleRightOfWay.txt");
        simulator->run();
    }
    catch (exception e)
    {
        cout << "ERROR: " << e.what();
    }

    return 0;
}
