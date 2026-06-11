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
        tests/Tests.cpp
        ${ADT_HEADERS}
        containers/BinaryHeap.h
        lazy/Generator.h
        lazy/LazySequenceHistoryView.h
        lazy/LazySequence.h
        lazy/LazySequenceProviders.h
        lazy/Ordinal.h
        statistics/OnlineStatisticsSnapshot.h
        streams/Streams.h
        streams/StreamExceptions.h
        streams/ReadOnlyStream.h
        statistics/OnlineStatistics.h
        streams/WriteOnlyStream.h
        tests/Tests.h)

set(PROJECT_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/adt
        ${CMAKE_CURRENT_SOURCE_DIR}/containers
        ${CMAKE_CURRENT_SOURCE_DIR}/lazy
        ${CMAKE_CURRENT_SOURCE_DIR}/statistics
        ${CMAKE_CURRENT_SOURCE_DIR}/streams
        ${CMAKE_CURRENT_SOURCE_DIR}/tests)

target_include_directories(lazysequence PRIVATE ${PROJECT_INCLUDE_DIRS})

add_executable(play
        examples/play.cpp
        ${ADT_HEADERS})

target_include_directories(play PRIVATE ${PROJECT_INCLUDE_DIRS})
