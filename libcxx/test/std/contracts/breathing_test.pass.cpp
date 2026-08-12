// ADDITIONAL_COMPILE_FLAGS: -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe -Xclang -fcontracts-group-evaluation-semantic=enforce:enforce -fcontracts-group-evaluation-semantic=quick_enforce:quick_enforce -fcontracts-group-evaluation-semantic=ignore:ignore -fcontracts-group-evaluation-semantic=observe:observe
#include <cassert>
#include <contracts>
#include <iostream>
#include <vector>
#include <compare>
#include <source_location>
#include <utility>
#include <tuple>
#include <fstream>
#include <unordered_map>
#include <format>
#include <vector>
#include <map>
#include <variant>
#include <regex>
#include <string>
#include <stdexcept>
#include <set>
#include "check_assertion.h"
#include "dump_struct.h"
#include "contracts_support.h"

int main() {}