#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include "sml_Client.h"
#include "State.h"
#include "Square.h"
#include "square_id_wme.h"

#include <vector>
#include <iostream>

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
        ofstream &soar_log); 

void dijkstra_update_il(
        const State &state,
        const Location &location,
        const char* attr_name,
        const vector<vector<int> > &values,
        Agent *agent, 
        vector<vector<SquareIdWME> > &grid_ids,
        vector<IntElement *> &temp_children,
        ostream &log);
#endif
