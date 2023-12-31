cmake_minimum_required(VERSION 3.22.1)

# The project name looks like we're building a library, but we're really building
# an executable. By naming the executable as if it were a library we can rely on
# the built in Android App Bundle functionality to ship the version for the
# correct ABI.
project("libfestival.so")

add_compile_definitions(LINUX)

add_executable(${CMAKE_PROJECT_NAME}
        advanced_deadlock.cpp
        advisors.cpp
        bfs.cpp
        biconnect.cpp
        board.cpp
        corral.cpp
        corral_deadlock.cpp
        cycle_deadlock.cpp
        deadlock.cpp
        deadlock_cache.cpp
        deadlock_utils.cpp
        debug.cpp
        diags.cpp
        distance.cpp
        dragonfly.cpp
        dragonfly_queue.cpp
        engine.cpp
        envelope.cpp
        fixed_boxes.cpp
        girl.cpp
        global.cpp
        graph.cpp
        greedy.cpp
        helper.cpp
        heuristics.cpp
        hf_search.cpp
        holes.cpp
        hotspot.cpp
        hungarian.cpp
        imagine.cpp
        io.cpp
        k_dist_deadlock.cpp
        lurd.cpp
        match_distance.cpp
        max_dist.cpp
        mini_search.cpp
        moves.cpp
        mpdb2.cpp
        naive.cpp
        oop.cpp
        opener.cpp
        order.cpp
        overlap.cpp
        packing_search.cpp
        park_order.cpp
        perimeter.cpp
        positions.cpp
        preprocess.cpp
        queue.cpp
        request.cpp
        rooms.cpp
        rooms_deadlock.cpp
        score.cpp
        snail.cpp
        sokoban_solver.cpp
        sol.cpp
        stuck.cpp
        textfile.cpp
        tree.cpp
        util.cpp
        wobblers.cpp
        xy_deadlock.cpp)
