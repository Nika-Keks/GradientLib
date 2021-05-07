#include <iostream>
#include <cmath>
#include "../include/IVector.h"
#include "../include/ILogger.h"
#include "../include/ISet.h"

#define epsilon 1e-8

void printVector(IVector* v){
    if (v == nullptr)
        std::cout << "null\n";
    double v_i;
    for (size_t i = 0; i < v->getDim(); i++)
    {
        v->getCord(i, v_i);
        std::cout << v_i << ' ' ;
    }
    std::cout << std::endl;
}

void testIVector(){
    ILogger* logger = ILogger::createLogger();
    IVector::setLogger(logger);

    double arr1[] = {1., 0.};
    double arr2[] = {0., 1.};

    IVector* v1 = IVector::createVector((size_t)2, arr1);
    IVector* v2 = IVector::createVector((size_t)2, arr2);

    printVector(v1);
    printVector(v2);
    IVector* v3 = IVector::sub(v1, v2);

    printVector(v3);
    std::cout << "RC v3->scale(10.) -> " << static_cast<int>(v3->scale(10.)) << std::endl;
    printVector(v3);
    IVector* v4 = v1->clone();
    printVector(v4);

    std::cout << IVector::equals(v1, v2, IVector::NORM::SECOND, 1.) << std::endl;
    std::cout << IVector::equals(v1, v1, IVector::NORM::SECOND, 1.) << std::endl;
    std::cout << std::endl;

    std::cout << v3->norm(IVector::NORM::FIRST) << std::endl;
    std::cout << v3->norm(IVector::NORM::SECOND) << std::endl;
    std::cout << v3->norm(IVector::NORM::CHEBYSHEV) << std::endl;

    std::cout << std::endl;
    IVector* v5 = v2->clone();
    printVector(v5);
    IVector::copyInstance(v5, v3);
    printVector(v5);
    IVector::moveInstance(v5, v2);
    printVector(v5);

    IVector::add(nullptr, nullptr);

    delete v1;
    delete v3;
    delete v4;
    delete v5;
    delete logger;
}

void printSet(ISet const* const& set){
    auto it = set->getBegin();
    if (it == nullptr) {
        std::cout << "it == nullprt" << std::endl;
        return;
    }
    std::cout << "set:" << std::endl;
    while (it->isValid()){
           IVector* vec;
           it->getVectorCopy(vec);
           printVector(vec);
           it->next();
    }
}

void testInsert(ISet* const& set, size_t dim, double const* const& data){
    auto vec = IVector::createVector(dim, data);
    std::cout << "insert ";
    printVector(vec);
    RC rc = set->insert(vec, IVector::NORM::SECOND, epsilon);
    std::cout << "RC\t" << static_cast<int>(rc) << std::endl;
    delete vec;
}

void testISet(){

    size_t dim = 3;
    double
    e1[] = { 1, 0, 0 },
    e2[] = { 0, 1, 0 },
    e3[] = { 0, 0, 1 },
    zero[] = {0, 0, 0},
    big[] = {0, 0, 0, 0},
    small[] = {0, 0};

    auto data = {e1, e2, e3, zero};
    auto set = ISet::createSet();
    for (auto arr : data){
         testInsert(set, dim, arr);
    }
    printSet(set);
}

