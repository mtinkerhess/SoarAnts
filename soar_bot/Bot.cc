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
                soar_log << "Making new ant agent for location " << my_ant_location->str() << endl;
                stringstream ant_name;
                ant_name << "ant-agent-" << next_ant_id;
                ++next_ant_id;
                ant_agent = new AntAgent(*my_ant_location, kernel, ant_name.str(), this);
                ant_agent->init_input_link(state, *my_ant_location);
                ant_agent->update_input_link(state, *my_ant_location, dijk_values, dijk_attr_names, num_dijk_values);
                direction = ant_agent->move(state, 0.0); // No reward on the first turn
            } else {
                // There exists an agent that was controlling this ant.
                // Let this agent move.
                soar_log << "Using existing agent for location " << my_ant_location->str() << endl;
                ant_agent = ant_agent_it->second;
                ant_agent->update_input_link(state, *my_ant_location, dijk_values, dijk_attr_names, num_dijk_values);
                direction = ant_agent->move(state, reward);
            }
            state.makeMove(*my_ant_location, direction);
            Location next_location = state.getLocation(*my_ant_location, direction);
            next_ant_agents->insert(make_pair(next_location, ant_agent));
            soar_log << "Moved in direction " << direction << ", expected in location " << next_location.str() << endl;
        }


        // Delete agents that didn't move this turn.
        // Also mark them as dead for good measure
        vector<Location> to_remove;
        for (map<Location, AntAgent*>::iterator ant_agent = ant_agents->begin();
                ant_agent != ant_agents->end(); ++ant_agent) {
            if (ant_agent->second->turn != state.turn) {
                soar_log << "Deleting obsolete agent " << ant_agent->second->name << endl;
                ant_agent->second->die();
                delete ant_agent->second; // Takes care of deleting the Soar agent via the kernel.
                to_remove.push_back(ant_agent->first);
            }
        }
        for (vector<Location>::const_iterator it = to_remove.begin();
                it != to_remove.end(); ++it) {
            ant_agents->erase(*it);
        }

        // Swap the locations map.
        // After this, ant_agents indexes ant by where they want to end up _next_ turn.
        delete ant_agents;
        ant_agents = next_ant_agents;


        //makeMoves();
        endTurn();
    }
}

//finishes the turn
void Bot::endTurn()
{
    if(state.turn > 0)
        state.reset();
    state.turn++;

    cout << "go" << endl;
};
