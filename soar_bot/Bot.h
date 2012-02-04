#ifndef BOT_H_
#define BOT_H_

#include <fstream>
#include "State.h"
#include "sml_Client.h"

using namespace sml;
using namespace std;

/*
    This struct represents your bot in the game of Ants
*/
struct Bot
{
    State state;

    Bot(const char *agent_name);
    ~Bot();

    Kernel *kernel;
    Agent *agent;

    ofstream soar_log;

    void playGame();    //plays a single game of Ants

    void makeMoves();   //makes moves for a single turn
    void endTurn();     //indicates to the engine that it has made its moves

    bool checkKernelError(Kernel *kernel);
    bool running;
    string agent_name;
};

#endif //BOT_H_
