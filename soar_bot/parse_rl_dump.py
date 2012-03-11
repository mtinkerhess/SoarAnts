#!/usr/bin/env python

rl_dump = 'rl_dump.soar'
template = 'rl_template.soar'
op_val = '<s> ^operator <o> = '
script_log = 'script_log.txt'

def average(values):
    return sum(values) / float(len(values))

def learns():
    keys = ('# Number of moves: ', '# Cumulatuve reward: ', '# Rule update: ')
    key_values = [0, 0, 0]
    rules = {}
    for line in open(rl_dump):
        for i, key in enumerate(keys):
            if key in line:
                if i == 2:
                    key_values[i] = int(line.strip()[len(key):].split()[1].split('.')[0])
                else:
                    key_values[i] = int(line.strip()[len(key):])
            elif 'sp {' in line:
                rule_name = line.strip()[4:]
            elif op_val in line:
                rule_value = line[line.find(op_val) + len(op_val):]
                rule_value = rule_value.split(')')[0]
                rule_value = float(rule_value)
                rules[rule_name] = rule_value
            elif '# RL rules for agent ' in line and len(rules) > 0:
                yield (key_values, rules)
                key_values = [0, 0, 0]
                rules = {}
    if len(rules) > 0:
        yield (key_values, rules)

def write_templates(rule_totals):
    read = {}
    rule_name = False
    for line in open(rl_dump):
        if 'sp {' in line:
            rule_name = line.split('{')[1].strip()
            if rule_name in read:
                rule_name = False
            else:
                read[rule_name] = []
        if rule_name:
            if op_val in line:
                val_index = line.find(op_val) + len(op_val)
                paren_index = line.find(')', val_index)
                line = line[:val_index] + str(average(rule_totals[rule_name])) + line[paren_index:]
            if line[0] != '#':
                read[rule_name].append(line.strip())
    with open(template, 'w') as f:
        for rule_name in read.keys():
            f.write('\n'.join(read[rule_name]))

def main():
    with open(script_log, 'a') as f:
        rule_totals = {}
        for learn in learns():
            f.write(str(learn) + '\n')
            for rule_name in learn[1].keys():
                if not rule_name in rule_totals:
                    rule_totals[rule_name] = []
                for i in range(learn[0][0]):
#                for i in range(learn[0][2]):
                    rule_totals[rule_name].append(learn[1][rule_name])
        for rule_name in rule_totals.keys():
            f.write(rule_name + ' ')
            f.write(str(average(rule_totals[rule_name])) + '\n')
        write_templates(rule_totals)

if __name__ == '__main__':
    main()
