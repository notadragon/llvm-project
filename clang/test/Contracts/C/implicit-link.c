// Verify that -fcontracts-p4299 alone links the program: the driver must pull
// in libcontracts (which provides __c_contract_check_*), with no explicit -l
// in the test.  This proves the flag pulls its dependency.  (%libcxx_flags is
// only a library-search-path substitution here, not an explicit library.)
//
// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=observe %libcxx_flags %s -o %t && %t

int f(int x) _Pre(x > 0) { return x; }

int main(void) { return f(1) == 1 ? 0 : 1; }
