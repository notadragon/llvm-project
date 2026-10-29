// RUN: %clang_cc1 -fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target

// P4405 stage 3 in C: a postcondition is one evaluation, however many
// returns its function has, and the proof shadow proves it at every return
// or diagnoses it once, at its declaration.  Falling off the end of a void
// function is a return too.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p4405-shadow-post.c

int sink(int);
int g;

int clamped(int v)
  _Post(r: r >= 0 && r <= 9)    // expected-remark {{proof shadow: proven, sound}}
{
  if (v < 0)
    return 0;
  if (v > 9)
    return 9;
  return v;
}

int partly(int v)
  _Post(r: r >= 0)              // expected-remark {{proof shadow: not proven}} \
                                // expected-error {{contract assertion cannot be proven}}
{
  if (v < 0)
    return v;
  return sink(v) & 7;
}

void store(int v)
  _Post(g >= 0)                 // expected-remark {{proof shadow: proven, sound}}
{
  if (v < 0) {
    g = 0;
    return;
  }
  g = v;
}
