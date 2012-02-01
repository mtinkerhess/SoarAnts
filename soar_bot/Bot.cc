#include "Bot.h"
#include "sml_Client.h"
#include <sstream>
#include <cstdlib>

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
Bot::Bot()
{
    soar_log.open("soar_log.txt");
    soar_log << "bot const" << endl;
    kernel = Kernel::CreateKernelInCurrentThread(Kernel::kDefaultLibraryName, true);
    checkKernelError(kernel);
    agent = kernel->CreateAgent("ants");
    checkKernelError(kernel);
    agent->RegisterForPrintEvent(smlEVENT_PRINT, printCallback, this);
    agent->ExecuteCommandLine("source ants.soar");
};

Bot::~Bot() {
    soar_log.close();
    kernel->Shutdown();
    delete kernel;
}

Identifier *make_child(Agent *agent, Identifier *parent, const char *name, int col, int row, vector<Identifier *> &temp_children) {
    Identifier *id = agent->CreateIdWME(parent, name);
    agent->CreateIntWME(id, "col", col);
    agent->CreateIntWME(id, "row", row);
    temp_children.push_back(id);
    return id;
}

//plays a single game of Ants.
void Bot::playGame()
{
    //reads the game parameters and sets up
    cin >> state;
    state.setup();

    // Init input link
    Identifier *il = agent->GetInputLink();
    IntElement *turn_id = agent->CreateIntWME(il, "turn", state.turn);
    vector<Identifier *> temp_children;

    Identifier *grid_id = agent->CreateIdWME(il, "grid");
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
            grid_ids[col].push_back(square_wme);
        }
    }
    // Make adjacency links
    for (int col = 0; col < state.cols; ++col) {
        for (int row = 0; row < state.rows; ++row) {
            // Left
            if (col > 0) {
                agent->CreateSharedIdWME(grid_ids[col][row].root, "left", grid_ids[col - 1][row].root);
            }
            // Right
            if (col + 1 < state.cols) {
                agent->CreateSharedIdWME(grid_ids[col][row].root, "right", grid_ids[col + 1][row].root);
            }
            // Up
            if (row > 0) {
                agent->CreateSharedIdWME(grid_ids[col][row].root, "up", grid_ids[col][row - 1].root);
            }
            // Down
            if (row + 1 < state.rows) {
                agent->CreateSharedIdWME(grid_ids[col][row].root, "down", grid_ids[col][row + 1].root);
            }
        }
    }

    endTurn();

    //continues making moves while the game is not over
    while(cin >> state)
    {
        state.updateVisionInformation();
        agent->Update(turn_id, state.turn);

        // Update input link
        for (int i = 0; i < temp_children.size(); ++i) {
            agent->DestroyWME(temp_children[i]);
        }
        temp_children.clear();
        for (int col = 0; col < state.cols; ++col) {
            for (int row = 0; row < state.rows; ++row) {
                Square &square = state.grid[row][col];
                
                // Update grid cell
                SquareIdWME &square_wme = grid_ids[col][row];
                square_wme.Update(square);
                // Update item wmes
                if (square.isWater) {
                    make_child(agent, il, "water", col, row, temp_children);
                }
                if (!square.isVisible) {
                    // Nothing else we can tell about this square.
                    continue;
                }
                if (square.isHill) {
                    Identifier *child = make_child(agent, il, "hill", col, row, temp_children);
                    agent->CreateIntWME(child, "player-id", square.hillPlayer);
                }
                if (square.isFood) {
                    make_child(agent, il, "food", col, row, temp_children);
                }
                if (square.ant >= 0) {
                    Identifier *child = make_child(agent, il, "ant", col, row, temp_children);
                    agent->CreateIntWME(child, "player-id", square.ant);
                }
            }
        }

        makeMoves();
        endTurn();
    }
};

//makes the bots moves for the turn
void Bot::makeMoves()
{
    soar_log << "making moves" << endl;
    state.bug << "turn " << state.turn << ":" << endl;
    state.bug << state << endl;

    bool done = false;
    while(!done) {

        soar_log << "running self" << endl;
        agent->RunSelfTilOutput();
        soar_log << "ran self" << endl;

        int num_commands = agent->GetNumberCommands();
        if (num_commands == 0) {
            soar_log << "ERROR, no commands" << endl;
            done = true;
        }
        for (int i = 0; i < num_commands; ++i) {
            Identifier *command = agent->GetCommand(i);
            string name = command->GetCommandName();
            soar_log << "Got command, name " << name << endl;
            if (name.compare("done") == 0) {
                soar_log << "name is done" << endl;
                done = true;
                soar_log << "Adding status complete, done" << endl;
                agent->CreateStringWME(command, "status", "complete");
            } else if (name.compare("move") == 0) {
                soar_log << "name is move" << endl;
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
                    soar_log << "Adding status error" << endl;
                    agent->CreateStringWME(command, "status", "error");
                    continue;
                }
                if (dir < 0) {
                    // stay
                    soar_log << "Adding status complete, stay" << endl;
                    agent->CreateStringWME(command, "status", "complete");
                    continue;
                }
                soar_log << " col " << col << " row " << row << " dir " << dir << endl;
                state.makeMove(Location(row, col), dir);
                soar_log << "Adding status complete" << endl;
                agent->CreateStringWME(command, "status", "complete");
            }
            soar_log << "outside test" << endl;
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
