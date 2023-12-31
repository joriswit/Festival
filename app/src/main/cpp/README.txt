
Festival 3.1 source code
========================

To compile the code for Windows, use the mingw64 compiler:

gcc -O3 *.cpp -pthread -DTHREADS

The source code is messy and undocumented. Festival was developed as a 
personal hobby project without the intention of releasing its code. 
I suspect that following the code will be very difficult for anyone else.

For those brave enough to try, here are some pointers:

The FESS algorithm is implemented in "engine.cpp" . The loop there goes over
the feature-space cells (which are called "requests" in the code), picks
a candidate from each cell and expands it.

The RL algorithm is implemented in girl.cpp and greedy.cpp ("girl" stands for
Greedy Iterations / Reinforcement Learning). 

There is also an implementation of a greedy Manhattan-Distance based search in
dragonfly.cpp (small brain but fast). This one is useful for small levels.

The main control loop is in "sokoban_solver.cpp". It runs eight searches: 
two of them are girl/dragonfly. The other six searches are FESS with 
different features being used.


Copyright issues:
The code contains copyright notices.
You can use the code, but if you do so please credit the program and its author.