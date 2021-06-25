#include "../include/ILogger.h"
#include <iostream>
#include <cmath>
#include <functional>
#include <map>
#include <functional>

void testIVector();

void testISet();

namespace comp{
    void testICompact();
}

//___________________________________
class IFoo{
public:
    virtual void foo() = 0;
};

class IBoo : public virtual IFoo{
public:
    virtual void boo() = 0;
};
//___________________________________

class FooImpl : public virtual IFoo{
protected:
    size_t i;
public:
    void foo() override{
        std::cout << "foo_impl" << std::endl;
    }
};

class BooImpl : public FooImpl, public IBoo{
public:
    void boo() override{
        std::cout << "boo_impl" << std::endl;
    }
};

int main()
{

    BooImpl b;
    b.foo();
    b.boo();
    //testIVector();

    //testISet();

    //comp::testICompact();

    return 0;
}
