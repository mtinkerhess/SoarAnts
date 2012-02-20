#include "square_id_wme.h"

#include "sml_Client.h"

using namespace std;
using namespace sml;

    void SquareIdWME::Update(const Square &square) {
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

SquareIdWME::SquareIdWME(
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
