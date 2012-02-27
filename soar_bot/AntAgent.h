#ifndef ANT_AGENT_H
#define ANT_AGENT_H

#include "sml_Client.h"

#include "square_id_wme.h"
#include "Bot.h"
#include "Square.h"

using namespace std;
using namespace sml;

/**
  * Represents a single agent.
  * This controls a single ant for the lifetime of that ant.
  * It uses its own Soar agent to make decisions about how to control that ant.
  * It gets passed a Soar kernel when it's constructed in order to make the agent.
  */
class AntAgent {
    private:
        // Passed in by constructor; doesn't own this.
        Kernel *kernel;
        Bot *bot;

        // Created in constructor.
        // Deleted in destructor.
        Agent *soar_agent;
    public:
        // The last turn that this ant moved
        int turn;

        // The last direction that this ant moved
        int last_move;

        // Is this ant alive?
        bool alive;

        // The expected location of the ant.
        Location location;

        string name;

        // Constructor
        AntAgent(Location location, Kernel *kernel, const string &name, Bot *bot);
        ~AntAgent();

        // Make a single move.
        // Arguments: the current state; reward from the previous move.
        // Returns: the direction to move in, or -1 if this ant has died.
        // 0 = N, 1 = E, 2 = S, 3 = W (same as in State.h)
        // Shouldn't be called until either init_input_link or update_input_link has been called this turn.
        int move(const State &state, double previous_reward);

        // Persistent input-link data structures
        Identifier *il;
        FloatElement *reward_wme;
        IntElement *turn_wme;
        vector<Identifier *> temp_children;
        vector<IntElement *> temp_int_children;
        Identifier *grid_id;
        vector<vector<SquareIdWME> > grid_ids;

        // Set up persistent input link structures.
        void init_input_link(const State &state, const Location &location);

        // Update the input link to reflect the currest state.
        void update_input_link(const State &state, const Location &location, const vector<vector<vector<int> > > &dijk_values, const string dijk_attr_names[], int num_dijk_values);

        // Call when then ant dies.
        void die();

};
#endif
