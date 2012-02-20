#include "AntAgent.h"
#include <vector>
#include <sstream>

#include "sml_Client.h"
#include "Dijkstra.h"
#include "Util.h"

using namespace std;
using namespace sml;

AntAgent::AntAgent(Location location, Kernel *kernel, const string &name, PrintEventHandler print_callback, Bot *bot) : kernel(kernel), bot(bot), turn(-1), alive(true), location(location),
    last_move(-1)
{
    soar_agent = kernel->CreateAgent(name.c_str());
    soar_agent->RegisterForPrintEvent(smlEVENT_PRINT, print_callback, bot);
}

AntAgent::~AntAgent() {
    kernel->DestroyAgent(soar_agent);
    soar_agent = NULL;
}

void AntAgent::update_input_link(const State &state, const Location &location, const vector<vector<vector<int> > > &dijk_values, const string dijk_attr_names[], int num_dijk_values) {
    bot->soar_log << "updating input link" << endl;
    this->location = location;
    bot->soar_log << "updated location" << endl;
    bot->soar_log << "new turn is: " << state.turn << endl;
    bot->soar_log << "soar_agent is: " << (long)(soar_agent) << endl;
    bot->soar_log << "turn_wme is: " << (long)(turn_wme) << endl;
    soar_agent->Update(turn_wme, state.turn);
    // Get rid of temp children from previous turn.
    bot->soar_log << "updated turn" << endl;
    for (int i = 0; i < temp_children.size(); ++i) {
        soar_agent->DestroyWME(temp_children[i]);
    }
    for (int i = 0; i < temp_int_children.size(); ++i) {
        soar_agent->DestroyWME(temp_int_children[i]);
    }
    temp_children.clear();
    temp_int_children.clear();
    bot->soar_log << "cleared old children" << endl;

    // Update dijkstra values.
    for (int value = 0; value < num_dijk_values; ++value) {
        dijkstra_update_il(state, dijk_attr_names[value].c_str(), dijk_values[value], soar_agent, grid_ids, temp_int_children);
    }
    bot->soar_log << "updated dijkstra values" << endl;
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
            IntElement *ant_id = soar_agent->CreateIntWME(root, "ant-id", -1);
            IntElement *hill_id = soar_agent->CreateIntWME(root, "hill-id", -2);
            SquareIdWME square_wme(root, is_visible, is_water, is_hill, is_food, ant_id, hill_id);
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
    if (!alive) return -1;
    soar_agent->Update(reward_wme, previous_reward);
    turn = state.turn;

    // TODO implement
    return rand_int(4);
}

// Called when this ant dies.
void AntAgent::die() {
    alive = false;
    // TODO put negative reward on input link
}
