#include "Bot.h"
#include "sml_Client.h"
#include <sstream>
#include <cstdlib>
#include <utility>
#include <ctime>
#include <exception>
#include "square_id_wme.h"
#include "Dijkstra.h"
#include "AntAgent.h"

using namespace sml;
using namespace std;

void print_callback(smlPrintEventId id, void* pUserData, Agent* pAgent, char const* pMessage) {
    (((Bot*) pUserData)->soar_log) << pMessage << endl;
}

bool Bot::checkKernelError(Kernel *kernel) {
    if (kernel->HadError()) {
        soar_log << kernel->GetLastErrorDescription() << endl;
        return true;
    }
    return false;
}

//constructor
Bot::Bot(const char *agent_name)
    : running(true), agent_name(agent_name)
{
    srand(time(NULL));
    soar_log.open("log.txt");
    soar_log << "bot ctor" << endl;
    soar_log << "agent name " << agent_name << endl;
    kernel = Kernel::CreateKernelInCurrentThread(Kernel::kDefaultLibraryName, true);
    checkKernelError(kernel);
    agent = kernel->CreateAgent("ants");
    checkKernelError(kernel);
    agent->RegisterForPrintEvent(smlEVENT_PRINT, print_callback, this);
    stringstream command;
    command << "source " << agent_name << ".soar";
    soar_log << agent->ExecuteCommandLine(command.str().c_str()) << endl;
    stringstream source_rl;
    source_rl << "source " << agent_name << "-rl.soar";
    soar_log << agent->ExecuteCommandLine(source_rl.str().c_str()) << endl;
};

Bot::~Bot() {
    soar_log.close();
    kernel->Shutdown();
    delete kernel;
}

// Functions for finding starting points for Dijkstra's algorithm
bool square_not_visible(const Square &square) { return !square.isVisible; }
bool square_is_water(const Square &square) { return square.isWater; }
bool square_is_my_hill(const Square &square) { return square.isHill && square.hillPlayer == 0; }
bool square_is_enemy_hill(const Square &square) { return square.isHill && square.hillPlayer != 0; }
bool square_is_food(const Square &square) { return square.isFood; }
bool square_is_my_ant(const Square &square) { return square.ant == 0; }
bool square_is_enemy_ant(const Square &square) { return square.ant > 0; }

//plays a single game of Ants.
void Bot::playGame()
{
    //reads the game parameters and sets up
    cin >> state;
    state.setup();

    // Init input link
    Identifier *il = agent->GetInputLink();
    FloatElement *reward_wme = agent->CreateFloatWME(il, "reward", 0.0f);
    IntElement *turn_id = agent->CreateIntWME(il, "turn", state.turn);
    vector<Identifier *> temp_children;
    vector<IntElement *> temp_int_children;

    Identifier *grid_id = agent->CreateIdWME(il, "grid");
    agent->CreateIntWME(grid_id, "cols", state.cols);
    agent->CreateIntWME(grid_id, "rows", state.rows);
    vector<vector<SquareIdWME> > grid_ids(state.cols);
    for (int col = 0; col < state.cols; ++col) {
        stringstream col_str;
        col_str << col;
        Identifier *col_id = agent->CreateIdWME(grid_id, col_str.str().c_str());
        for (int row = 0; row < state.rows; ++row) {
            stringstream row_str;
            row_str << row;
            Identifier *root = agent->CreateIdWME(col_id, row_str.str().c_str());
            StringElement *is_visible = agent->CreateStringWME(root, "visible", "false");
            StringElement *is_water = agent->CreateStringWME(root, "water", "false");
            StringElement *is_hill = agent->CreateStringWME(root, "hill", "false");
            StringElement *is_food = agent->CreateStringWME(root, "food", "false");
            IntElement *ant_id = agent->CreateIntWME(root, "ant-id", -1);
            IntElement *hill_id = agent->CreateIntWME(root, "hill-id", -2);
            SquareIdWME square_wme(root, is_visible, is_water, is_hill, is_food, ant_id, hill_id);
            agent->CreateIntWME(root, "col", col);
            agent->CreateIntWME(root, "row", row);
            grid_ids[col].push_back(square_wme);
        }
    }
    // Make adjacency links
    for (int col = 0; col < state.cols; ++col) {
        for (int row = 0; row < state.rows; ++row) {
                agent->CreateSharedIdWME(grid_ids[col][row].root, "left", grid_ids[(col - 1 + state.cols) % state.cols][row].root);
                agent->CreateSharedIdWME(grid_ids[col][row].root, "right", grid_ids[(col + 1) % state.cols][row].root);
                agent->CreateSharedIdWME(grid_ids[col][row].root, "up", grid_ids[col][(row - 1 + state.rows) % state.rows].root);
                agent->CreateSharedIdWME(grid_ids[col][row].root, "down", grid_ids[col][(row + 1) % state.rows].root);
        }
    }

    endTurn();

    int num_ants = 1;


    soar_log << "About to make dijkstra structures" << endl;

    // Maps locations onto the ant agent that controls the ant at that location.
    map<Location, AntAgent *> *ant_agents = new map<Location, AntAgent *>;

    int next_ant_id = 0;
    // Perform Dijkstra's algorithm.
    static const string dijk_attr_names[] = {
        "distance-to-not-visible",
        "distance-to-water",
        "distance-to-my-hill",
        "distance-to-enemy-hill",
        "distance-to-food",
        "distance-to-my-ant",
        "distance-to-enemy-ant"
    };

    static bool (*dijk_funcs[])(const Square &) = {
        square_not_visible,
        square_is_water,
        square_is_my_hill,
        square_is_enemy_hill,
        square_is_food,
        square_is_my_ant,
        square_is_enemy_ant
    };
    int num_dijk_values = 7;

    soar_log << "about to start first turn" << endl;

    //continues making moves while the game is not over
    while(cin >> state)
    {
        state.updateVisionInformation();
        int d_ants = state.myAnts.size() - num_ants;
        num_ants = state.myAnts.size();
        double reward = -1 + d_ants * 10;

        vector<vector<vector<int> > > dijk_values(num_dijk_values);
        for (int value = 0; value < num_dijk_values; ++value) {
            dijkstras_algorithm(state, dijk_funcs[value], dijk_values[value], soar_log);
            // Do this in each agent
            // dijkstra_update_il(dijk_attr_names[value], dijk_values[value], agent, grid_ids, temp_int_children);
        }

        map<Location, AntAgent *> *next_ant_agents = new map<Location, AntAgent *>; // Will later replace ant_agents

        for (vector<Location>::const_iterator my_ant_location = state.myAnts.begin();
                my_ant_location != state.myAnts.end(); ++my_ant_location) {
            map<Location, AntAgent *>::iterator ant_agent_it = ant_agents->find(*my_ant_location);
            int direction = -1;
            AntAgent *ant_agent = NULL;
            if (ant_agents->empty() || ant_agent_it == ant_agents->end()) {
                // There is no ant that was trying to move to this location.
                // Create a new agent for that ant.
                stringstream ant_name;
                ant_name << "ant-agent-" << next_ant_id;
                ++next_ant_id;
                ant_agent = new AntAgent(*my_ant_location, kernel, ant_name.str(), print_callback, this);
                ant_agent->init_input_link(state, *my_ant_location);
                ant_agent->update_input_link(state, *my_ant_location, dijk_values, dijk_attr_names, num_dijk_values);
                direction = ant_agent->move(state, 0.0); // No reward on the first turn
            } else {
                // There exists an agent that was controlling this ant.
                // Let this agent move.
                ant_agent = ant_agent_it->second;
                ant_agent->update_input_link(state, *my_ant_location, dijk_values, dijk_attr_names, num_dijk_values);
                direction = ant_agent->move(state, reward);
            }
            state.makeMove(*my_ant_location, direction);
            next_ant_agents->insert(make_pair(state.getLocation(*my_ant_location, direction), ant_agent));
        }


        // Delete agents that didn't move this turn.
        // Also mark them as dead for good measure
        for (map<Location, AntAgent*>::iterator ant_agent = ant_agents->begin();
                ant_agent != ant_agents->end(); ++ant_agent) {
            if (ant_agent->second->turn != state.turn) {
                ant_agent->second->die();
                delete ant_agent->second; // Takes care of deleting the Soar agent via the kernel.
            }
        }


        // Swap the locations map.
        // After this, ant_agents indexes ant by where they want to end up _next_ turn.
        delete ant_agents;
        ant_agents = next_ant_agents;


        //makeMoves();
        endTurn();
    }
    /*
    soar_log << agent->ExecuteCommandLine("stats") << endl;
    stringstream ctf_command;
    ctf_command << "ctf " << agent_name << "-rl.soar print --full --rl";
    soar_log << "About to write back rl rules: " << ctf_command.str() << endl;
    soar_log << agent->ExecuteCommandLine(ctf_command.str().c_str()) << endl;
    soar_log << "print --rl" << endl;
    soar_log << agent->ExecuteCommandLine("print --rl") << endl;
    soar_log << "fc" << endl;
    soar_log << agent->ExecuteCommandLine("fc") << endl;
    */
}

//makes the bots moves for the turn
// DEPRECATED
void Bot::makeMoves()
{
    state.bug << "turn " << state.turn << ":" << endl;
    state.bug << state << endl;

    bool done = false;
    while(running && !done) {

        agent->RunSelfTilOutput();

        int num_commands = agent->GetNumberCommands();
        if (num_commands == 0) {
            soar_log << "ERROR, no commands" << endl;
            if (running) {
                running = false;
                soar_log << agent->ExecuteCommandLine("pref -n s1") << endl;
                soar_log << agent->ExecuteCommandLine("p -d 10 <s>") << endl;
            }
            done = true;
        }
        for (int i = 0; i < num_commands; ++i) {
            Identifier *command = agent->GetCommand(i);
            string name = command->GetCommandName();
            if (name.compare("done") == 0) {
                done = true;
                agent->CreateStringWME(command, "status", "complete");
            } else if (name.compare("move") == 0) {
                string col_str = command->GetParameterValue("col");
                string row_str = command->GetParameterValue("row");
                string dir_str = command->GetParameterValue("direction");
                int col = atoi(col_str.c_str());
                int row = atoi(row_str.c_str());
                int dir = -1;
                if (dir_str.compare("up") == 0) {
                    dir = 0;
                } else if (dir_str.compare("right") == 0) {
                    dir = 1;
                } else if (dir_str.compare("down") == 0) {
                    dir = 2;
                } else if (dir_str.compare("left") == 0) {
                    dir = 3;
                } else if (dir_str.compare("stay") == 0) {
                    dir = -2;
                }
                if (dir == -1) {
                    agent->CreateStringWME(command, "status", "error");
                    continue;
                }
                if (dir < 0) {
                    // stay
                    agent->CreateStringWME(command, "status", "complete");
                    continue;
                }
                state.makeMove(Location(row, col), dir);
                agent->CreateStringWME(command, "status", "complete");
            }
        }
    }
    
    //picks out moves for each ant
    /*
    for(int ant=0; ant<(int)state.myAnts.size(); ant++)
    {
        for(int d=0; d<TDIRECTIONS; d++)
        {
            Location loc = state.getLocation(state.myAnts[ant], d);

            if(!state.grid[loc.row][loc.col].isWater)
            {
                state.makeMove(state.myAnts[ant], d);
                break;
            }
        }
    }
    */

    state.bug << "time taken: " << state.timer.getTime() << "ms" << endl << endl;
};

//finishes the turn
void Bot::endTurn()
{
    if(state.turn > 0)
        state.reset();
    state.turn++;

    cout << "go" << endl;
};
