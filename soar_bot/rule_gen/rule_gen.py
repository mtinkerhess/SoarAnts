#!/usr/bin/env python

def generate_layer_points(layer):

    """Generates a set of points in the layer-th layer of a perception.
    Points are generated in concentric circles going outward.
    The circle with index 0 is the center of the perception--the ant itself."""
    
    # First, find the set of (unsigned) offsets from the center.
    # e.g., if layer = 3, offsets = [(0, 2), (1, 1), (2, 0)]
    # If layer = -1, offsets = [(0, 0)] <-- obsolete
    offsets = [(i, layer - i) for i in range(layer + 1)]

    # Take the offsets and create a set of points with them.
    # Do this for all four permutations of positive and negative x and y.
    # Use a dict-set to prevent duplicates.
    points = {}
    signs = (-1, 1)
    for x_sign in signs:
        for y_sign in signs:
            for offset in offsets:
                point = (offset[0] * x_sign, offset[1] * y_sign)
                points[point] = point

    # Return a tuple representation of the points.
    return tuple(points.keys())

def generate_points(radius):
    """Generates all the points in a perception vector of range=radius."""
    return reduce(lambda x, y: x + y, (generate_layer_points(layer + 1) for layer in range(radius)), ())

def num_perpception_layer_points(layer):
    """Returns the number of points in the layer-th layer of a perecption."""
    return (layer + 1) * 4

def num_perception_points(radius):
    """Returns the number of points in a perception of range=radius."""
    return sum(num_perpception_layer_points(layer) for layer in range(radius))

def num_perceptions(radius, num_values):
    """Returns the number of possible perceptions of range=radius,
    if the number of values each point can take is equal to num_values."""
    return pow(num_values, num_perception_points(radius))

def convert_base(value, base):
    """Returns a list representation of the result of converting
    the base-10 _value_ to a new representation in base-_base_.
    Assumes that value >= 0, and is an int.
    Note that the least significant values are at the beginning of the return value."""
    place = 1
    ret = []
    while (value > 0):
        next_place = place * base
        next_value = value - (value % next_place)
        ret.append((value - next_value) / place)
        value = next_value
        place = next_place
    return ret

def generate_index_perception(index, radius, num_values):
    """Generates a simplified version of the index-th perception vector of range=radius,
    with the possible number of values for each cell=num_values.
    Returns a list of values."""

    # This can be framed as returning the number in base-num_values
    # that represents the number _index_.
    # Fill in extra zeros.
    converted_base = convert_base(index, num_values)
    ret_length = num_perception_points(radius)
    return converted_base + [0 for i in range(ret_length - len(converted_base))]

def generate_perception(index, radius, num_values):
    """Generates the index-th perception vector of range=radius,
    with the possible number of values for each cell=num_values.
    Returns a list of tuples, where each tuple is in the form (x, y, value, index)."""

    index_perceptions = generate_index_perception(index, radius, num_values)
    points = generate_points(radius)
    return [(points[i][0], points[i][1], index_perceptions[i]) for i in range(len(points))]

def generate_perceptions(radius, num_values):
    number_perceptions = num_perceptions(radius, num_values)
    print number_perceptions
    return (generate_perception(index, radius, num_values) for index in range(num_perceptions(radius, num_values)))

def make_perception_dict(perception):
    """Builds a dict to store the perception in.
    Each entry is of the form ret[(x, y)] = value."""
    ret = {}
    for percept in perception:
        ret[(percept[0], percept[1])] = percept[2]
    return ret

def is_symmetrical(perception):
    points = {}
    for percept in perception:
        abs_x = abs(percept[0])
        y = percept[1]
        value = percept[2]
        abs_point = (abs_x, y)
        if not abs_point in points:
            points[abs_point] = value
        else:
            if points[abs_point] != value:
                return False
    return True

def is_canonical(perception, radius, num_values):
    """Returns True if this is the 'canonical' representation of this perception.
    e.g., if this is not the canonical representation it should be flipped over
    the y-axis (negate all x-values) in order to become the canonical representation.
    The idea is to allow agents to generalize over perceptions that are symmetrical to each other."""

    perception_dict = make_perception_dict(perception)

    # Do this messy-like
    # Compare right-hand side to left-hand side
    for x in range(1, radius + 1):
        for y in range(-radius, radius + 1):
            if x + abs(y) > radius:
                continue
            if (x, y) not in perception_dict or (-x, y) not in perception_dict:
                print 'ERROR, point not in dict'
            right_value = perception_dict[(x, y)]
            left_value = perception_dict[(-x, y)]
            if right_value > left_value:
                return True
            if right_value < left_value:
                return False

    # The perception is symmetrical
    return True

def are_mirror(first, second, radius, num_values):
    """Returns true if for all x, for all y,
    first[x][y] == second[-x][y]."""

    first_dict = make_perception_dict(first)
    second_dict = make_perception_dict(second)

    # Do this messy-like
    # Compare right-hand side to left-hand side
    for x in range(-radius, radius + 1):
        for y in range(-radius, radius + 1):
            if x == 0 and y == 0:
                continue
            print x, y, radius
            if abs(x) + abs(y) > radius:
                continue
            if (x, y) not in first_dict or (x, y) not in second_dict:
                print 'ERROR, point not in first or second dict'
            if first_dict[(x, y)] != second_dict[(-x, y)]:
                return False

    # The perceptions are mirror
    return True

def generate_canonical_perceptions(radius, num_values):
    """Returns two parallel lists.
    The first is a list of perceptions.
    The second is the index of the corresponding canonical perception.
    For many perceptions, the canonical index will be its own index."""
    perceptions = list(generate_perceptions(radius, num_values))
    canonical_indexes = [-1 for i in range(len(perceptions))]
    for i, perception in enumerate(perceptions):
        if is_canonical(perception, radius, num_values):
            canonical_indexes[i] = i
        else:
            # Find the canonical representation of this perception.
            # e.g., find the perception that is a mirror of this perception.
            for j in range(len(perceptions)):
                if i == j:
                    continue
                if are_mirror(perception, perceptions[j], radius, num_values):
                    canonical_indexes[i] = j
                    break
    return perceptions, canonical_indexes

def make_perception_attr_val(percept, values):
    """Returns an attribute-value rule fragment that
    tests for the presence of this particular percept.
    Percept is a single item in a perception."""
    x = percept[0]
    y = percept[1]
    value = percept[2]
    if value == 0:
        return False
    value_attr = values[value - 1][0]
    value_val = values[value - 1][1]
    parts = []
    x_dir = '<left>' if x < 0 else '<right>'
    y_dir = '<forward>' if y > 0 else '<back>'
    return '^' + '.'.join([x_dir for i in range(abs(x))] + [y_dir for i in range(abs(y))] + [value_attr,]) + ' ' + value_val

def make_template(index, perceptions, canonical_indexes, values):
    """Returns a string containing a template rule for the perception at
    the given index.
    Values is a list of 2-tuples that map to the attribute-values at each square.
    The length of values is num_values - 1, because if value == 0 that means
    we don't care about what's in that square."""

    # The purpose of these rules are to flag the operator with a canonical-index,
    # then to use that index as the LHS of an RL rule.

    parts = []
    parts.append('sp {ants*move-template*' + str(index) + '-of-' + str(len(perceptions)) + '\n')
    parts.append('   (state <s> ^name ants ^operator <o> +)\n')
    parts.append('   (<o> ^name move ^location <loc>)\n')

    # TODO make sure these tags are getting generated
    parts.append('   (<o> ^forward <forward> ^back <back> ^left <left> ^right <right>)\n')

    for percept in perceptions[index]:
        fragment = make_perception_attr_val(percept, values)
        if not fragment:
            continue
        parts.append('   (<loc> ' + fragment + ')\n')

    parts.append('-->\n')
    parts.append('   (<o> ^canonical-index ' + str(canonical_indexes[index]) + ')\n}\n\n')
    parts.append('sp {ants*move-template*' + str(index) + '-of-' + str(len(perceptions)) + '*rl\n')
    parts.append('   (state <s> ^name ants ^operator <o> +)\n')
    parts.append('   (<o> ^name move canonical-index ' + str(canonical_indexes[index]) + ')\n')
    parts.append('-->\n')
    parts.append('   (<s> ^operator <o> = 0.0)\n}\n')
    return ''.join(parts)

def make_templates_from_perceptions(perceptions, canonical_indexes, values):
    return (make_template(index, perceptions, canonical_indexes, values) for index in range(len(perceptions)))

def make_templates(radius, values):
    num_values = len(values) + 1
    perceptions, canonical_indexes = generate_canonical_perceptions(radius, num_values)
    return make_templates_from_perceptions(perceptions, canonical_indexes, values)

def main():
    radius = 5
    values = (('ant.player-id', '<> 0'), ('ant.player-id', '0'))
    for template in make_templates(radius, values):
        print template

if __name__ == '__main__':
    main()
