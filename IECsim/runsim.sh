#!/bin/bash
RUN=$1
make fusorsim
nohup ./fusorsim 2>&1 | tee ${RUN}/fusorsim.log
