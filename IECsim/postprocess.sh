#!/bin/bash
RUN=$1
source venv/bin/activate
cp fusorsim.cpp ${RUN}/fusorsimrun${RUN}.cpp
python pot_radial_all.py
python traj_distribution.py
python pot_error.py
python centralheight.py
python epot_error_rms.py
python memory_estimate.py
