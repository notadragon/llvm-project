struct B { virtual ~B() {} };
struct E : B { E(); };
struct F : E { int x = 0; };
E::E() { (void)static_cast<F*>(static_cast<B*>(this)); }  // defined while an F is being built
int main() { F f; }
