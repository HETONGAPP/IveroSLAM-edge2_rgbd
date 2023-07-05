#!/usr/bin/env python2
"""
Save keyframes to folder given a keyframe trajectory
"""
import sys
from collections import OrderedDict
import numpy as np
from os import listdir
from os.path import isfile, join
import math

def main():
    args = sys.argv[1:]
    if(len(args) != 2):
        print("Usage: python3 get_rgb_kf_trajectory.py /path_to_kf_trajectory /path_to_interpolated_trajectory")
        return

    # 1. read rgb image folder, store as list of timestamps
    # 2. read trajectory file, store as an ordereddict of poses (4x4)
    # 3. for each rgb image timestamp, interpolate its pose, convert to tum format, output to new file
    
    kf_times = set()
    kf_trajectory_file = args[0]
    with open(kf_trajectory_file) as file:
        for line in file:
            values = line.rstrip().split()
            timestamp = float(values[0])
            kf_times.add(timestamp)

    interpolated_trajectory = {}
    interp_trajectory_file = args[1]
    time_to_filename = {}
    with open(interp_trajectory_file) as file:
        for line in file:
            values = line.rstrip().split()
            timestamp = float(values[0])*1e-9
            params = [float(values[1]), float(values[2]), float(values[3]), float(values[4]), float(values[5]), float(values[6]), float(values[7])]
            time_to_filename[timestamp] = values[0]
            interpolated_trajectory[timestamp] = params
    
    sorted_kfs = sorted(kf_times)
    outfile = kf_trajectory_file.rstrip('.txt') + '-rgb.txt'
    file_out = open(outfile, 'w')

    used_rgb_frames=set()
    for kf in sorted_kfs:
        res_key, tr = min(interpolated_trajectory.items(), key=lambda x: abs(kf - x[0]))
        if res_key in used_rgb_frames: 
            continue
        used_rgb_frames.add(res_key)
        params_out = time_to_filename[res_key] + " " + f'{tr[0]:.20f}' + " " + f'{tr[1]:.20f}' + " " + f'{tr[2]:.20f}' + " " + f'{tr[3]:.20f}' + " " + f'{tr[4]:.20f}' + " " + f'{tr[5]:.20f}' + " " + f'{tr[6]:.20f}' + "\n"
        file_out.write(params_out)



if __name__ == '__main__':
    main()
