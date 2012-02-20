#include "util.h"

#include <vector>
#include <cstdlib>

#include "sml_Client.h"
#include "square_id_wme.h"

using namespace std;
using namespace sml;

// Creates a new identifier from the given parent, with the given name.
// Gives the idetifier col and row WMEs.
// Makes WMEs pointing to and from the grid square to the new item, with the given name.
// Adds these to the temp_children array so that someone else can clean them up later.
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

double rand_double() {
    return rand() / double(RAND_MAX);
}

int rand_int(int range) {
    return int(rand_double() * range);
}
