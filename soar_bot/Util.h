#ifndef UTIL_H
#define UTIL_H

#include <vector>

#include "sml_Client.h"

#include "square_id_wme.h"

using namespace std;
using namespace sml;

// Creates a new identifier from the given parent, with the given name.
// Gives the idetifier col and row WMEs.
// Makes WMEs pointing to and from the grid square to the new item, with the given name.
// Adds these to the temp_children array so that someone else can clean them up later.
Identifier *make_child(Agent *agent, Identifier *parent, const char *name, int col, int row, vector<Identifier *> &temp_children, vector<vector<SquareIdWME> > &grid_ids);

double rand_double();
int rand_int(int range);

#endif
