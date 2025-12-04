#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <queue>
#include <chrono>
#include <algorithm>
#include <fstream>

#include "tinyxml2.h"


using namespace std;
using Marking = std::vector<int>;

// Conditional compilation for CUDD library
#ifndef NO_CUDD
    #include <cudd.h>
#else
    using DdManager = void;
    using DdNode = void;
#endif