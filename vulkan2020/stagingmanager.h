/*
 *
 * Andrew Frost
 * staging.h
 * 2020
 *
 */

#pragma once
#include "header.h"
#include "renderbackend.h"

#ifndef __STAGING_H__
#define __STAGING_H__

struct stagingBuffer_t {

};

class StagingManager {
public:
    StagingManager();
    ~StagingManager();

    void init();
    void close();

    std::byte* stage();

    void flush();

private:
    void wait();

private:

};

#endif