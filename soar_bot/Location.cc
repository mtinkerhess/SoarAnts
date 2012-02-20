#include "Location.h"

bool operator<(const Location &left, const Location &right) {
        if (left.col < right.col) return true;
        if (left.col > right.col) return false;
        return left.row < right.row;
}
