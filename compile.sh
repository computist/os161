#!bin/sh

cd ~/cse451
cd os161/kern/conf
./config ASST3
cd ~/cse451
cd os161/kern/compile/ASST3/
bmake depend
bmake
bmake install
