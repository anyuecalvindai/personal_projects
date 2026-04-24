#!/bin/bash
RUN=$1
mkdir -p ${RUN}/plots
sed -i "s/string run = \"[^\"]*\"/string run = \"${RUN}\/\"/" fusorsim.cpp
sed -i "s|run = '[^']*'|run = '${RUN}/'|" traj_distribution.py
sed -i "s|run = '[^']*'|run = '${RUN}/'|" pot_radial_all.py
sed -i "s|run = '[^']*'|run = '${RUN}/'|" pot_error.py
sed -i "s|run = '[^']*'|run = '${RUN}/'|" centralheight.py
sed -i "s|run = '[^']*'|run = '${RUN}/'|" epot_error_rms.py
sed -i "s|run = '[^']*'|run = '${RUN}/'|" memory_estimate.py