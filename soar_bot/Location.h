#ifndef LOCATION_H_
#define LOCATION_H_

#include <string>
#include <sstream>

using namespace std;

/*
    struct for representing locations in the grid.
*/
struct Location
{
    int row, col;

    Location()
    {
        row = col = 0;
    };

    Location(int r, int c)
    {
        row = r;
        col = c;
    };

    string str() const {
        stringstream ss;
        ss << "Location (" << col << ", " << row << ")";
        return ss.str();
    }
};

bool operator<(const Location &left, const Location &right);


#endif //LOCATION_H_
