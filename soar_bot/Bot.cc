#include "Bot.h"
#include "sml_Client.h"
#include <sstream>
#include <cstdlib>
#include <utility>
#include <ctime>

using namespace sml;
using namespace std;

struct SquareIdWME {
    Identifier *root;
    StringElement *is_visible;
    StringElement *is_water;
    StringElement *is_hill;
    StringElement *is_food;
    IntElement *ant_player_id;
    IntElement *hill_player_id;

    bool was_visible;
    bool was_water;
    bool was_hill;
    bool was_food;
    int was_ant;
    int was_hill_player;

    void Update(const Square &square) {
        if (square.isVisible != was_visible) {
            is_visible->Update(square.isVisible ? "true" : "false");
            was_visible = square.isVisible;
        }
        if (square.isWater != was_water) {
            is_water->Update(square.isWater ? "true" : "false");
            was_water = square.isWater;
        }
        if (square.isHill != was_hill) {
            is_hill->Update(square.isHill ? "true" : "false");
            was_hill = square.isHill;
        }
        if (square.isFood != was_food) {
            is_food->Update(square.isFood ? "true" : "false");
            was_food = square.isFood;
        }
        if (square.ant != was_ant) {
            ant_player_id->Update(square.ant);
            was_ant = square.ant;
        }
        if (square.hillPlayer != was_hill_player) {
            hill_player_id->Update(square.hillPlayer);
            was_hill_player = square.hillPlayer;
        }
    }



    SquareIdWME(
            Identifier *root,
            StringElement *is_visible,
            StringElement *is_water,
            StringElement *is_hill,
            StringElement *is_food,
            IntElement *ant_player_id,
            IntElement *hill_player_id)
        :
            root(root),
            is_visible(is_visible),
            is_water(is_water),
            is_hill(is_hill),
            is_food(is_food),
            ant_player_id(ant_player_id),
            hill_player_id(hill_player_id),

            was_visible(false),
            was_water(false),
            was_hill(false),
            was_food(false),
            was_ant(-1),
            was_hill_player(-1)
    { }
};

void printCallback(smlPrintEventId id, void* pUserData, Agent* pAgent, char const* pMessage) {
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
    soar_log.open("log.txt");
    soar_log << "bot ctor" << endl;
    soar_log << "agent name " << agent_name << endl;
    kernel = Kernel::CreateKernelInCurrentThread(Kernel::kDefaultLibraryName, true);
    checkKernelError(kernel);
    agent = kernel->CreateAgent("ants");
    checkKernelError(kernel);
    agent->RegisterForPrintEvent(smlEVENT_PRINT, printCallback, this);
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

Identifier *make_child(Agent *agent, Identifier *parent, const char *name, int col, int row, vector<Identifier *> &temp_children, vector<vector<SquareIdWME> > &grid_ids) {
    Identifier *id = agent->CreateIdWME(parent, name);
    agent->CreateIntWME(id, "col", col);
    agent->CreateIntWME(id, "row", row);
    Identifier *id2 = agent->CreateSharedIdWME(id, "square", grid_ids[col][row].root);
    Identifier *id3 = agent->CreateSharedIdWME(grid_ids[col][row].root, name, id);
    temp_children.push_back(id);
    temp_children.push_back(id2);
    temp_children.push_back(id3);
    return id;
}

// Functions for finding starting points for Dijkstra's algorithm
bool square_not_visible(const Square &square) { return !square.isVisible; }
bool square_is_water(const Square &square) { return square.isWater; }
bool square_is_my_hill(const Square &square) { return square.isHill && square.hillPlayer == 0; }
bool square_is_enemy_hill(const Square &square) { return square.isHill && square.hillPlayer != 0; }
bool square_is_food(const Square &square) { return square.isFood; }
bool square_is_my_ant(const Square &square) { return square.ant == 0; }
bool square_is_enemy_ant(const Square &square) { return square.ant > 0; }

clock_t dijk_timers[3];

// Perform Dijkstra's algorithm on the state, 
// Annotating the input link representation with
// information about the distance to certain kinds
// of features.
bool dijkstras_algorithm(Agent *&agent,
        vector<vector<SquareIdWME> > &grid_ids,
        const State &state,
        bool (*square_func)(const Square &square),
        const char *attr_name,
        vector<IntElement *> &temp_children,
        ofstream &soar_log) {

    // Create a data structure to hold the distance values in.
    vector<vector<int> > values(state.cols, vector<int>(state.rows, -1));

    // Find all squares that satisfay the initial condition.
    // Mark their values as 0.
    // If none are found, return false.
    dijk_timers[0] -= clock();
    int squares_found = 0;
    for (int col = 0; col < state.cols; ++col) {
        for (int row = 0; row < state.rows; ++row) {
            if (square_func(state.grid[row][col])) {
                ++squares_found;
                values[col][row] = 0;
            }
        }
    }
    dijk_timers[0] += clock();
    if (squares_found == 0) {
        return false;
    }

    // Propogate values.
    // For 0 up to some maximum value,
    // for each square marked with that value,
    // for each of its neighbors that are still marked -1 and aren't water,
    // mark that neighbor with one more than the value.
    // Keep track of how many neighbors are marked;
    // if no neighbors are marked, terminate propogation early.
    int max_value = state.cols + state.rows;
    static const pair<int, int> left = make_pair(-1, 0);
    static const pair<int, int> right = make_pair(1, 0);
    static const pair<int, int> up = make_pair(0, -1);
    static const pair<int, int> down = make_pair(0, 1);
    static const pair<int, int> directions[] = {left, right, up, down};
    squares_found = 1; // hack
    dijk_timers[1] -= clock();
    for (int value = 0; value < max_value && squares_found != 0; ++value) {
        squares_found = 0;
        for (int col = 0; col < state.cols; ++col) {
            for (int row = 0; row < state.rows; ++row) {
                if (values[col][row] == value) {
                    for (int direction = 0; direction < 4; ++direction) {
                        int d_col = directions[direction].first;
                        int d_row = directions[direction].second;
                        int other_col = (col + d_col + state.cols) % state.cols;
                        int other_row = (row + d_row + state.rows) % state.rows;
                        if ((!state.grid[row][col].isWater || value == 0) && values[other_col][other_row] == -1) {
                            ++squares_found;
                            values[other_col][other_row] = value + 1;
                        }
                    }
                }
            }
        }
    }
    dijk_timers[1] += clock();

    // Mark WMEs.
    dijk_timers[2] -= clock();
    for (int col = 0; col < state.cols; ++col) {
        for (int row = 0; row < state.rows; ++row) {
            int value = values[col][row];
            if (value < 0) continue;
            IntElement *wme = agent->CreateIntWME(grid_ids[col][row].root, attr_name, values[col][row]);
            temp_children.push_back(wme);
        }
    }
    dijk_timers[2] += clock();
    return true;
}

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
            // Left
                agent->CreateSharedIdWME(grid_ids[col][row].root, "left", grid_ids[(col - 1 + state.cols) % state.cols][row].root);
            // Right
                agent->CreateSharedIdWME(grid_ids[col][row].root, "right", grid_ids[(col + 1) % state.cols][row].root);
            // Up
                agent->CreateSharedIdWME(grid_ids[col][row].root, "up", grid_ids[col][(row - 1 + state.rows) % state.rows].root);
            // Down
                agent->CreateSharedIdWME(grid_ids[col][row].root, "down", grid_ids[col][(row + 1) % state.rows].root);
        }
    }

    endTurn();

    // For timing information
    int num_timers = 8;
    vector<clock_t> timers(num_timers, 0);

    int num_ants = 1;

    soar_log << "about to start first turn" << endl;

    //continues making moves while the game is not over
    while(cin >> state)
    {
        state.updateVisionInformation();
        agent->Update(turn_id, state.turn);

        int d_ants = state.myAnts.size() - num_ants;
        num_ants = state.myAnts.size();

        double reward = -1 + d_ants * 10;

        soar_log << "new turn: " << state.turn << endl;
        soar_log << "reward was " << reward << endl;

        // Update input link
        agent->Update(reward_wme, reward);
        for (int i = 0; i < temp_children.size(); ++i) {
            agent->DestroyWME(temp_children[i]);
        }
        for (int i = 0; i < temp_int_children.size(); ++i) {
            agent->DestroyWME(temp_int_children[i]);
        }
        temp_children.clear();
        temp_int_children.clear();
        for (int col = 0; col < state.cols; ++col) {
            for (int row = 0; row < state.rows; ++row) {
                Square &square = state.grid[row][col];
                
                // Update grid cell
                SquareIdWME &square_wme = grid_ids[col][row];
                square_wme.Update(square);
                // Update item wmes
                if (square.isWater) {
                    make_child(agent, il, "water", col, row, temp_children, grid_ids);
                }
                if (!square.isVisible) {
                    // Nothing else we can tell about this square.
                    continue;
                }
                if (square.isHill) {
                    Identifier *child = make_child(agent, il, "hill", col, row, temp_children, grid_ids);
                    agent->CreateIntWME(child, "player-id", square.hillPlayer);
                }
                if (square.isFood) {
                    make_child(agent, il, "food", col, row, temp_children, grid_ids);
                }
                if (square.ant >= 0) {
                    Identifier *child = make_child(agent, il, "ant", col, row, temp_children, grid_ids);
                    agent->CreateIntWME(child, "player-id", square.ant);
                }
            }
        }

        // Perform Dijkstra's algorithm.
        timers[0] -= clock();
        dijkstras_algorithm(agent, grid_ids, state, square_not_visible, "distance-to-not-visible", temp_int_children, soar_log);
        timers[0] += clock();
        timers[1] -= clock();
        dijkstras_algorithm(agent, grid_ids, state, square_is_water, "distance-to-water", temp_int_children, soar_log);
        timers[1] += clock();
        timers[2] -= clock();
        dijkstras_algorithm(agent, grid_ids, state, square_is_my_hill, "distance-to-my-hill", temp_int_children, soar_log);
        timers[2] += clock();
        timers[3] -= clock();
        dijkstras_algorithm(agent, grid_ids, state, square_is_enemy_hill, "distance-to-enemy-hill", temp_int_children, soar_log);
        timers[3] += clock();
        timers[4] -= clock();
        dijkstras_algorithm(agent, grid_ids, state, square_is_food, "distance-to-food", temp_int_children, soar_log);
        timers[4] += clock();
        timers[5] -= clock();
        dijkstras_algorithm(agent, grid_ids, state, square_is_my_ant, "distance-to-my-ant", temp_int_children, soar_log);
        timers[5] += clock();
        timers[6] -= clock();
        dijkstras_algorithm(agent, grid_ids, state, square_is_enemy_ant, "distance-to-enemy-ant", temp_int_children, soar_log);
        timers[6] += clock();

        timers[7] -= clock();
        makeMoves();
        timers[7] += clock();
        endTurn();
    }
    soar_log << agent->ExecuteCommandLine("stats") << endl;
    for (int i = 0; i < num_timers; ++i) {
        soar_log << "timer " << i << ": " << (timers[i] / double(CLOCKS_PER_SEC)) << endl;
    }
    for (int i = 0; i < 3; ++i) {
        soar_log << "dijk timer " << i << ": " << (dijk_timers[i] / double(CLOCKS_PER_SEC)) << endl;
    }
    stringstream ctf_command;
    ctf_command << "ctf " << agent_name << "-rl.soar print --full --rl";
    soar_log << "About to write back rl rules: " << ctf_command.str() << endl;
    soar_log << agent->ExecuteCommandLine(ctf_command.str().c_str()) << endl;
    soar_log << "print --rl" << endl;
    soar_log << agent->ExecuteCommandLine("print --rl") << endl;
    soar_log << "fc" << endl;
    soar_log << agent->ExecuteCommandLine("fc") << endl;
}

//makes the bots moves for the turn
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
