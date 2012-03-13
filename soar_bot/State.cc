#include "State.h"

using namespace std;

//constructor
State::State()
{
    gameover = 0;
    turn = 0;
    bug.open("./debug.txt");
};

//deconstructor
State::~State()
{
    bug.close();
};

//sets the state up
void State::setup()
{
    grid = vector<vector<Square> >(rows, vector<Square>(cols, Square()));
};

//resets all non-water squares to land and clears the bots ant vector
void State::reset()
{
    myAnts.clear();
    enemyAnts.clear();
    myHills.clear();
    enemyHills.clear();
    food.clear();
    for(int row=0; row<rows; row++) {
        for(int col=0; col<cols; col++) {
            grid[row][col].isDestination = false;
            if(!grid[row][col].isWater) {
                grid[row][col].reset();
            }
        }
    }
};

//outputs move information to the engine
void State::makeMove(const Location &loc, int direction)
{
    cout << "o " << loc.row << " " << loc.col << " " << CDIRECTIONS[direction] << endl;

    Location nLoc = getLocation(loc, direction);
    grid[nLoc.row][nLoc.col].ant = grid[loc.row][loc.col].ant;
    grid[loc.row][loc.col].ant = -1;
    grid[nLoc.row][nLoc.col].isDestination = true;
};

//returns the euclidean distance between two locations with the edges wrapped
double State::distance(const Location &loc1, const Location &loc2)
{
    int d1 = abs(loc1.row-loc2.row),
        d2 = abs(loc1.col-loc2.col),
        dr = min(d1, rows-d1),
        dc = min(d2, cols-d2);
    return sqrt(dr*dr + dc*dc);
};

//returns the new location from moving in a given direction with the edges wrapped
Location State::getLocation(const Location &loc, int direction)
{
    if (direction < 0) return loc;
    return Location( (loc.row + DIRECTIONS[direction][0] + rows) % rows,
                     (loc.col + DIRECTIONS[direction][1] + cols) % cols );
};

/*
    This function will update update the lastSeen value for any squares currently
    visible by one of your live ants.

    BE VERY CAREFUL IF YOU ARE GOING TO TRY AND MAKE THIS FUNCTION MORE EFFICIENT,
    THE OBVIOUS WAY OF TRYING TO IMPROVE IT BREAKS USING THE EUCLIDEAN METRIC, FOR
    A CORRECT MORE EFFICIENT IMPLEMENTATION, TAKE A LOOK AT THE GET_VISION FUNCTION
    IN ANTS.PY ON THE CONTESTS GITHUB PAGE.
*/
void State::updateVisionInformation()
{
    std::queue<Location> locQueue;
    Location sLoc, cLoc, nLoc;

    for(int a=0; a<(int) myAnts.size(); a++)
    {
        sLoc = myAnts[a];
        locQueue.push(sLoc);

        std::vector<std::vector<bool> > visited(rows, std::vector<bool>(cols, 0));
        grid[sLoc.row][sLoc.col].isVisible = 1;
        visited[sLoc.row][sLoc.col] = 1;

        while(!locQueue.empty())
        {
            cLoc = locQueue.front();
            locQueue.pop();

            for(int d=0; d<TDIRECTIONS; d++)
            {
                nLoc = getLocation(cLoc, d);

                if(!visited[nLoc.row][nLoc.col] && distance(sLoc, nLoc) <= viewradius)
                {
                    grid[nLoc.row][nLoc.col].isVisible = 1;
                    locQueue.push(nLoc);
                }
                visited[nLoc.row][nLoc.col] = 1;
            }
        }
    }
};

/*
    This is the output function for a state. It will add a char map
    representation of the state to the output stream passed to it.

    For example, you might call "cout << state << endl;"
*/
ostream& operator<<(ostream &os, const State &state)
{
    for(int row=0; row<state.rows; row++)
    {
        for(int col=0; col<state.cols; col++)
        {
            if(state.grid[row][col].isWater)
                os << '%';
            else if(state.grid[row][col].isFood)
                os << '*';
            else if(state.grid[row][col].isHill)
                os << (char)('A' + state.grid[row][col].hillPlayer);
            else if(state.grid[row][col].ant >= 0)
                os << (char)('a' + state.grid[row][col].ant);
            else if(state.grid[row][col].isVisible)
                os << '.';
            else
                os << '?';
        }
        os << endl;
    }

    return os;
};

//input function
istream& operator>>(istream &is, State &state)
{
    int row, col, player;
    string inputType, junk;

    //finds out which turn it is
    while(is >> inputType)
    {
        if(inputType == "end")
        {
            state.gameover = 1;
            break;
        }
        else if(inputType == "turn")
        {
            is >> state.turn;
            break;
        }
        else //unknown line
            getline(is, junk);
    }

    if(state.turn == 0)
    {
        //reads game parameters
        while(is >> inputType)
        {
            if(inputType == "loadtime")
                is >> state.loadtime;
            else if(inputType == "turntime")
                is >> state.turntime;
            else if(inputType == "rows")
                is >> state.rows;
            else if(inputType == "cols")
                is >> state.cols;
            else if(inputType == "turns")
                is >> state.turns;
            else if(inputType == "player_seed")
                is >> state.seed;
            else if(inputType == "viewradius2")
            {
                is >> state.viewradius;
                state.viewradius = sqrt(state.viewradius);
            }
            else if(inputType == "attackradius2")
            {
                is >> state.attackradius;
                state.attackradius = sqrt(state.attackradius);
            }
            else if(inputType == "spawnradius2")
            {
                is >> state.spawnradius;
                state.spawnradius = sqrt(state.spawnradius);
            }
            else if(inputType == "ready") //end of parameter input
            {
                state.timer.start();
                break;
            }
            else    //unknown line
                getline(is, junk);
        }
    }
    else
    {
        //reads information about the current turn
        while(is >> inputType)
        {
            if(inputType == "w") //water square
            {
                is >> row >> col;
                state.grid[row][col].isWater = 1;
            }
            else if(inputType == "f") //food square
            {
                is >> row >> col;
                state.grid[row][col].isFood = 1;
                state.food.push_back(Location(row, col));
            }
            else if(inputType == "a") //live ant square
            {
                is >> row >> col >> player;
                state.grid[row][col].ant = player;
                if(player == 0) {
                    state.myAnts.push_back(Location(row, col));
                }
                else { 
                    state.enemyAnts.push_back(Location(row, col));
                }
            }
            else if(inputType == "d") //dead ant square
            {
                is >> row >> col >> player;
                state.grid[row][col].deadAnts.push_back(player);
            }
            else if(inputType == "h")
            {
                is >> row >> col >> player;
                state.grid[row][col].isHill = 1;
                state.grid[row][col].hillPlayer = player;
                if(player == 0)
                    state.myHills.push_back(Location(row, col));
                else
                    state.enemyHills.push_back(Location(row, col));

            }
            else if(inputType == "players") //player information
                is >> state.noPlayers;
            else if(inputType == "scores") //score information
            {
                state.scores = vector<double>(state.noPlayers, 0.0);
                for(int p=0; p<state.noPlayers; p++)
                    is >> state.scores[p];
            }
            else if(inputType == "go") //end of turn input
            {
                if(state.gameover)
                    is.setstate(std::ios::failbit);
                else
                    state.timer.start();
                break;
            }
            else //unknown line
                getline(is, junk);
        }
    }

    return is;
};

// Heloper funciton for State::getAttackOpponents.
// Tells whether an offset is within some range.
bool inRange(int d_row, int d_col, int range) {
    return sqrt(static_cast<double>(d_row * d_row + d_col * d_col)) <= range;
}

// Gets the number of opponents of the given player within the attack radius of the given location.
// Returns a lower bound unless upperBound is true, in which case returns an upper bound.
int State::getAttackOpponents(int loc_row, int loc_col, int playerId, bool upperBound) const {
    int ret = 0;

    static const int directions[5][2] = {{0, 0}, {0, 1}, {0, -1}, {1, 0}, {-1, 0}};
    static const int num_directions = 5;

    // Loop over attack range + 1 to simplify in case we want an upper bound.
    for (int d_row = -attackradius - 1; d_row <= attackradius + 1; ++d_row) {
        for (int d_col = -attackradius - 1; d_col <= attackradius + 1; ++d_col) {
            int row = (loc_row + d_row + rows) % rows;
            int col = (loc_col + d_col + cols) % cols;
            // If there is an opponent ant here, see if the ant is within the given bounds,
            // Depending on: upperBound = true / false; square.isDestination
            if (grid[row][col].ant >= 0 && grid[row][col].ant != playerId) {
                // There is an opponent ant in this square.
                // 2 cases: the ant has moved or it hasn't.
                if (grid[row][col].isDestination) {
                    // The opponent ant has moved 
                    // Check the range.
                    if (inRange(d_col, d_row, attackradius)) {
                        ++ret;
                    }
                } else {
                    // The opponent ant hasn't moved
                    // Check to see if any adjacent square is in range.
                    // If not, assume the square itself is not in range.
                    bool allInRange = true;
                    bool someInRange = false;
                    for (int direction = 0; direction < num_directions; ++direction) {
                        if (inRange(d_row + directions[direction][0], d_col + directions[direction][1], attackradius)) {
                            someInRange = true;
                        } else {
                            allInRange = false;
                        }
                    }
                    // Depends on upperBound
                    if ((upperBound && someInRange) || (!upperBound && allInRange)) {
                        ++ret;
                    }
                }
            }
        }
    }
}

// Gets a list of the locations that an enemy of this player might be at next turn that are
// within the attack radius.
vector<pair<int, int> > State::getPossibleOpponentLocations(int loc_row, int loc_col, int playerId) const {
    static const int directions[5][2] = {{0, 0}, {0, 1}, {0, -1}, {1, 0}, {-1, 0}};
    static const int num_directions = 5;
    vector<pair<int, int> > ret;
    for (int d_row = -attackradius - 1; d_row <= attackradius + 1; ++d_row) {
        for (int d_col = -attackradius - 1; d_col <= attackradius + 1; ++d_col) {
            int row = (loc_row + d_row + rows) % rows;
            int col = (loc_col + d_col + cols) % cols;
            if (grid[row][col].ant >= 0 && grid[row][col].ant != playerId) {
                if (grid[row][col].isDestination) {
                    // The opponent ant has moved 
                    // Check the range.
                    if (inRange(d_col, d_row, attackradius)) {
                        ret.push_back(make_pair(row, col));
                    }
                } else {
                    // The opponent ant hasn't moved
                    // Check to see if any adjacent square is in range.
                    // Also check the opponent's square itself.
                    for (int direction = 0; direction < num_directions; ++direction) {
                        if (inRange(d_row + directions[direction][0], d_col + directions[direction][1], attackradius)) {
                            int adj_row = (row + d_row + rows) % rows;
                            int adj_col = (col + d_col + cols) % cols;
                            ret.push_back(make_pair(adj_row, adj_col));
                        }
                    }
                }
 
            }
        }
    }
    return ret;
}
