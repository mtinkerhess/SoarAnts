#include "Dijkstra.h"

#include <vector>

#include "sml_Client.h"

#include "Square.h"

using namespace std;
using namespace sml;

// Perform Dijkstra's algorithm on the state, 
// Annotating the input link representation with
// information about the distance to certain kinds
// of features.
// WARNING: clears values.
void dijkstras_algorithm(
        const State &state,
        bool (*square_func)(const Square &square),
        vector<vector<int> > &values,
        ofstream &soar_log) {

    // Create a data structure to hold the distance values in.
//    vector<vector<int> > values(state.cols, vector<int>(state.rows, -1));
    values.clear();
    values.resize(state.cols, vector<int>(state.rows, -1));

    // Find all squares that satisfay the initial condition.
    // Mark their values as 0.
    // If none are found, return.
    int squares_found = 0;
    for (int col = 0; col < state.cols; ++col) {
        for (int row = 0; row < state.rows; ++row) {
            if (square_func(state.grid[row][col])) {
                ++squares_found;
                values[col][row] = 0;
            }
        }
    }
    if (squares_found == 0) {
        return;
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

}

void dijkstra_update_il(
        const State &state,
        const char* attr_name,
        const vector<vector<int> > &values,
        Agent *agent, 
        vector<vector<SquareIdWME> > &grid_ids,
        vector<IntElement *> &temp_children) {
    // Mark WMEs.
    for (int col = 0; col < state.cols; ++col) {
        for (int row = 0; row < state.rows; ++row) {
            int value = values[col][row];
            if (value < 0) continue;
            IntElement *wme = agent->CreateIntWME(grid_ids[col][row].root, attr_name, values[col][row]);
            temp_children.push_back(wme);
        }
    }
}
