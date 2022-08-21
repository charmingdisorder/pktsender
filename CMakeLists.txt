cmake_minimum_required(VERSION 3.0)
project(pktsender)

add_compile_options(-Wall -Wextra -pedantic -g)

set(SENDER_SOURCE_FILES pkt_sender.c md5.c)

add_executable(pkt_sender ${SENDER_SOURCE_FILES})
