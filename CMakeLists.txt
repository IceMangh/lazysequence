cmake_minimum_required(VERSION 4.0)
project(lazysequence)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ADT_HEADERS
        adt/ArraySequenceBase.h
        adt/DynamicArray.h
        adt/Exceptions.h
        adt/ICollection.h
        adt/IEnumerable.h
        adt/IEnumerator.h
        adt/MutableArraySequence.h
        adt/Sequence.h)

add_executable(lazysequence
        main.cpp
        Tests.cpp
        ${ADT_HEADERS}
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

target_include_directories(lazysequence PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/adt)

add_executable(play
        play.cpp
        ${ADT_HEADERS})

target_include_directories(play PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/adt)
