// Header for p3595-header-template.cpp: a function template, a class
// template's member and an inline function, all with a precondition, all
// matched by the configuration's "location" entry for this file.
template <class T> T hdr_f(T x) pre(x > 0) { return x; }
template <class T> struct HdrS { T g(T x) pre(x > 0) { return x; } };
inline int hdr_inline(int x) pre(x > 0) { return x; }
