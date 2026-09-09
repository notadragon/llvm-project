// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3099 %libcxx_flags \
// RUN:   -fsyntax-only -Xclang -verify

// P3099: redeclaration sameness is based on the extracted message *text*, not
// the expression structure.  A constexpr message that produces different text
// on different lines (via source_location) must be diagnosed; one that produces
// identical text must be accepted.
// (GCC mirror: g++.dg/contracts/cpp26/p3099-message-redecl-cexpr.C)
//
// (Previously custom-type messages were blocked, then a further defect blocked
// redeclaration message-sameness.  Both are now fixed; sameness compares the
// extracted text.)

#include <source_location>

struct LocMsg {
    char buf[64];
    constexpr LocMsg(std::source_location loc = std::source_location::current())
        : buf{} {
        int line = loc.line();
        buf[0] = 'L';
        buf[1] = '0' + (line / 10);
        buf[2] = '0' + (line % 10);
        buf[3] = '\0';
    }
    constexpr int size() const { int i = 0; while (buf[i]) ++i; return i; }
    constexpr const char* data() const { return buf; }
};

// Same expression, but different extracted text due to different lines.
void f(int x) pre(x > 0, LocMsg{}); // expected-note {{contract previously specified with a different diagnostic message}}
void f(int x) pre(x > 0, LocMsg{}); // expected-error {{differs in contract specifier sequence}} expected-note {{in contract specified here}}

// Same expression on separate lines, same extracted text (no line dependency).
struct FixedMsg {
    constexpr int size() const { return 5; }
    constexpr const char* data() const { return "hello"; }
};

void g(int x) pre(x > 0, FixedMsg{});
void g(int x) pre(x > 0, FixedMsg{}); // OK, same text
