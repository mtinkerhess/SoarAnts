#include "AntAgent.h"
#include <vector>
#include <sstream>

#include "sml_Client.h"
#include "Dijkstra.h"
#include "Util.h"

using namespace std;
using namespace sml;

AntAgent::AntAgent(Location location, Kernel *kernel, const string &name, Bot *bot) : kernel(kernel), bot(bot), turn(-1), alive(true), location(location),
    last_move(-1), name(name)
{
    soar_agent = kernel->CreateAgent(name.c_str());
    stringstream source_command;
    source_command << "source ants.soar";
    soar_agent->ExecuteCommandLine(source_command.str().c_str());
    soar_agent->RegisterForPrintEvent(smlEVENT_PRINT, print_callback, &(bot->soar_log));
    bot->soar_log << "Registered for print event callback" << endl;
}

AntAgent::~AntAgent() {
    kernel->DestroyAgent(soar_agent);
    soar_agent = NULL;
}

/**
  * Updates the input link to reflect the currest state,
  * from the perspective of a particular ant.
  *
  * state: The state of the world that's been read from cin.
  * location: The location of this ant.
  * dijk_values: Propogated dijkstra's algorithm values for this turn, not specific to this ant.
  * dijk_attr_names: Parallel vector to dijk_values, contains names of the attributes.
  * num_dijk_values: How many dijk values there are (could this be inferred from dijk_values.size()?)
  */
void AntAgent::update_input_link(const State &state, const Location &location, const vector<vector<vector<int> > > &dijk_values, const string dijk_attr_names[], int num_dijk_values) {
    bot->soar_log << "updating input link for " << name << endl;
    this->location = location;
    bot->soar_log << "new turn is: " << state.turn << endl;
    soar_agent->Update(turn_wme, state.turn);
    // Get rid of temp children from previous turn.
    for (int i = 0; i < temp_children.size(); ++i) {
        soar_agent->DestroyWME(temp_children[i]);
    }
    for (int i = 0; i < temp_int_children.size(); ++i) {
        soar_agent->DestroyWME(temp_int_children[i]);
    }
    temp_children.clear();
    temp_int_children.clear();

    // Update grid flags
//    for (int col = 0; col < state.cols; ++col) {
//        for (int row = 0; row < state.rows; ++row) {
    for (int d_col = -1; d_col <= 1; ++d_col) {
        for (int d_row = -1; d_row <= 1; ++d_row) {
            int col = (location.col + d_col) % state.cols;
            int row = (location.row + d_row) % state.rows;
            const Square &square = state.grid[row][col];

            // Update grid cell
            SquareIdWME &square_wme = grid_ids[col][row];
            square_wme.Update(square);
            // Update item wmes
            if (square.isWater) {
                bot->soar_log << "Making water tag: " << col << ", " << row << endl;
                make_child(soar_agent, il, "water", col, row, temp_children, grid_ids);
            }
            if (!square.isVisible) {
                // Nothing else we can tell about this square.
                continue;
            }
            if (square.isHill) {
                Identifier *child = make_child(soar_agent, il, "hill", col, row, temp_children, grid_ids);
                soar_agent->CreateIntWME(child, "player-id", square.hillPlayer);
            }
            if (square.isFood) {
                make_child(soar_agent, il, "food", col, row, temp_children, grid_ids);
            }
            if (square.ant >= 0) {
                if (d_col != 0 || d_row != 0) {
                    bot->soar_log << "Making ant tag: " << col << ", " << row << endl;
                }
                Identifier *child = make_child(soar_agent, il, "ant", col, row, temp_children, grid_ids);
                soar_agent->CreateIntWME(child, "player-id", square.ant);
            }
        }
    }

    // Update dijkstra values.
    for (int value = 0; value < num_dijk_values; ++value) {
        dijkstra_update_il(state, location, dijk_attr_names[value].c_str(), dijk_values[value], soar_agent, grid_ids, temp_int_children, bot->soar_log);
    }

    // Update the ant's current location
    Identifier *me_id = soar_agent->CreateSharedIdWME(il, "me", grid_ids[location.col][location.row].root);
    temp_children.push_back(me_id);
}

void AntAgent::init_input_link(const State &state, const Location &location) {
    il = soar_agent->GetInputLink();
    reward_wme = soar_agent->CreateFloatWME(il, "reward", 0.0f);
    turn_wme = soar_agent->CreateIntWME(il, "turn", state.turn);
    grid_id = soar_agent->CreateIdWME(il, "grid");
    grid_ids.resize(state.cols);
    for (int col = 0; col < state.cols; ++col) {
        stringstream col_str;
        col_str << col;
        Identifier *col_id = soar_agent->CreateIdWME(grid_id, col_str.str().c_str());
        for (int row = 0; row < state.rows; ++row) {
            stringstream row_str;
            row_str << row;
            Identifier *root = soar_agent->CreateIdWME(col_id, row_str.str().c_str());
            StringElement *is_visible = soar_agent->CreateStringWME(root, "visible", "false");
            StringElement *is_water = soar_agent->CreateStringWME(root, "water", "false");
            StringElement *is_hill = soar_agent->CreateStringWME(root, "hill", "false");
            StringElement *is_food = soar_agent->CreateStringWME(root, "food", "false");
            StringElement *is_destination = soar_agent->CreateStringWME(root, "destination", "false"); // Whether an ant is planning on moving here
            IntElement *ant_id = soar_agent->CreateIntWME(root, "ant-id", -1);
            IntElement *hill_id = soar_agent->CreateIntWME(root, "hill-id", -2);
            SquareIdWME square_wme(root, is_visible, is_water, is_hill, is_food, is_destination, ant_id, hill_id);
            soar_agent->CreateIntWME(root, "col", col);
            soar_agent->CreateIntWME(root, "row", row);
            grid_ids[col].push_back(square_wme);
        }
    }
    // Make adjacency links
    for (int col = 0; col < state.cols; ++col) {
        for (int row = 0; row < state.rows; ++row) {
            soar_agent->CreateSharedIdWME(grid_ids[col][row].root, "left", grid_ids[(col - 1 + state.cols) % state.cols][row].root);
            soar_agent->CreateSharedIdWME(grid_ids[col][row].root, "right", grid_ids[(col + 1) % state.cols][row].root);
            soar_agent->CreateSharedIdWME(grid_ids[col][row].root, "up", grid_ids[col][(row - 1 + state.rows) % state.rows].root);
            soar_agent->CreateSharedIdWME(grid_ids[col][row].root, "down", grid_ids[col][(row + 1) % state.rows].root);
        }
    }
}

// Shouldn't be called until either init_input_link or update_input_link has been called this turn.
int AntAgent::move(const State &state, double previous_reward) {
    bot->soar_log << "MOVE called on ant " << name << endl;
    if (!alive) return -1;
    soar_agent->Update(reward_wme, previous_reward);
    turn = state.turn;

    soar_agent->RunSelfTilOutput();
    int num_commands = soar_agent->GetNumberCommands();
    if (num_commands == 0) {
        bot->soar_log << "ERROR, no commands" << endl;
        bot->soar_log << soar_agent->ExecuteCommandLine("pref -n s1") << endl;
        bot->soar_log << soar_agent->ExecuteCommandLine("p -d 10 <s>") << endl;
    }
    int ret = -1;
    for (int command_index = 0; command_index < num_commands; ++command_index) {
        Identifier *command_id = soar_agent->GetCommand(command_index);
        string command_name = command_id->GetCommandName();
        string col_str = command_id->GetParameterValue("col");
        string row_str = command_id->GetParameterValue("row");
        string direction_str = command_id->GetParameterValue("direction");
        int col;
        int row;
        istringstream col_buffer(col_str);
        istringstream row_buffer(row_str);
        col_buffer >> col;
        row_buffer >> row;
        int direction = -1;
        if (direction_str.compare("up") == 0) {
            direction = 0;
        } else if (direction_str.compare("right") == 0) {
            direction = 1;
        } else if (direction_str.compare("down") == 0) {
            direction = 2;
        } else if (direction_str.compare("left") == 0) {
            direction = 3;
        } else if (direction_str.compare("stay") == 0) {
            direction = -2;
        }
        bot->soar_log << "Got command from output link: " << command_name << ", col " << col << ", row " << row << ", direction " << direction << endl;
        ret = direction;
        command_id->AddStatusComplete();
    }
    return ret;
}

// Called when this ant dies.
void AntAgent::die() {
    alive = false;
    // TODO put negative reward on input link
}
