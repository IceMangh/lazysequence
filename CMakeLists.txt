cmake_minimum_required(VERSION 4.0)
project(lazysequence)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lazysequence
        main.cpp
        Tests.cpp
        BinaryHeap.h
        DynamicArraySequenceView.h
        Generator.h
        LazySequence.h
        Ordinal.h
        OnlineStatisticsSnapshot.h
        Streams.h
        StreamExceptions.h
        ReadOnlyStream.h
        OnlineStatistics.h
        WriteOnlyStream.h
        Tests.h)

target_include_directories(lazysequence PRIVATE ../atd-next)

add_executable(play
        play.cpp)

target_include_directories(play PRIVATE ../atd-next)
