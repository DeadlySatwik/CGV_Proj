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
    cout << "   Controls: " << endl << endl;
    cout << " W,A,S,D       - camera movement" << endl;
    cout << " Q, E / Space  - vertical movement" << endl;
    cout << " dragging cursor - rotating camera" << endl;
    cout << endl;
    cout << " F             - toggle 3rd person car view" << endl;
    cout << " Arrow / IJKL  - drive (in 3rd person mode)" << endl;
    cout << " H             - honk horn (in 3rd person mode)" << endl;
    cout << " P             - attempt to park (in Training Mode)" << endl;
    cout << " 1, 2, 3       - switch vehicle (Car, Bus, Bike)" << endl;
    cout << endl;
    cout << " R             - cycle Game Modes (Basic, Training, Exam)" << endl;
    cout << " V             - cycle Lessons (in Training Mode)" << endl;
    cout << " O             - toggle Projection (Perspective / Orthographic)" << endl;
    cout << endl;
    cout << " T, Y          - decrease/increase updates per frame" << endl;
    cout << " [, ]          - decrease/increase time scale" << endl;
    cout << " N, M          - next day phase / toggle time flow" << endl;
    cout << " I             - print telemetry" << endl;
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
