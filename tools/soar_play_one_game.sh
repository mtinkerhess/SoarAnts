#!/usr/bin/env sh
./playgame.py --player_seed 42 --end_wait=1.0 --verbose --log_dir game_logs --turns 200 --map_file maps/maze/maze_04p_01.map --turntime 60000 --html replay.html --nolaunch "$@" "../soar_bot/SoarBot" "python sample_bots/python/RandomBot.py" "python sample_bots/python/RandomBot.py" "python sample_bots/python/RandomBot.py"
