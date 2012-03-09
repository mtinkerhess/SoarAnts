#!/usr/bin/env python

from os import system
import os

def main():
    num_rounds = 100
    try:
        os.remove('script_log.txt')
    except:
        pass
    try:
        os.remove('rl_template.soar')
    except:
        pass
    # system('touch rl_template.soar')
    for i in range(num_rounds):
        try:
            os.remove('rl_dump.soar')
        except:
            pass
        old_dir = os.getcwd()
        os.chdir('../tools')
        system('./soar_play_one_game.sh')
        os.chdir(old_dir)
        system('./parse_rl_dump.py')

if __name__ == '__main__':
    main()
