#include <iostream>
#include <cmath>
#include <vector>
#include "../include/IVector.h"
#include "../include/ILogger.h"
#include "../include/ISet.h"
#include "../include/ICompact.h"

#define epsilon 1e-8


double const
        e1[] = { 1, 0, 0 },
        e2[] = { 0, 1, 0 },
        e3[] = { 0, 0, 1 },
        zero[] = {0, 0, 0},
        v1[] = {1, 1, 1},
        v2[] = {2, 2, 2},
        v3[] = {3, 3, 3},
        v4[] = {4, 4, 4},
        v5[] = {5, 5, 5},
        big[] = {0, 0, 0, 0},
        small[] = {0, 0};
std::vector<double const*> vectors ={ zero, e1, e2, e3, v1, v2, v3, v4, v5 };
size_t const dim = 3;

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

    IVector* v_1 = IVector::createVector((size_t)2, arr1);
    IVector* v_2 = IVector::createVector((size_t)2, arr2);

    printVector(v_1);
    printVector(v_2);
    IVector* v_3 = IVector::sub(v_1, v_2);

    printVector(v_3);
    std::cout << "RC v_3->scale(10.) -> " << static_cast<int>(v_3->scale(10.)) << std::endl;
    printVector(v_3);
    IVector* v_4 = v_1->clone();
    printVector(v_4);

    std::cout << IVector::equals(v_1, v_2, IVector::NORM::SECOND, 1.) << std::endl;
    std::cout << IVector::equals(v_1, v_1, IVector::NORM::SECOND, 1.) << std::endl;
    std::cout << std::endl;

    std::cout << v_3->norm(IVector::NORM::FIRST) << std::endl;
    std::cout << v_3->norm(IVector::NORM::SECOND) << std::endl;
    std::cout << v_3->norm(IVector::NORM::CHEBYSHEV) << std::endl;

    std::cout << std::endl;
    IVector* v_5 = v_2->clone();
    printVector(v_5);
    IVector::copyInstance(v_5, v_3);
    printVector(v_5);
    IVector::moveInstance(v_5, v_2);
    printVector(v_5);

    IVector::add(nullptr, nullptr);

    delete v_1;
    delete v_3;
    delete v_4;
    delete v_5;
    delete logger;
}

void printSet(ISet const* const& set){
    if (set == nullptr){
        std::cout << "set == nullptr" << std::endl;
        return;
    }
    auto it = set->getEnd();
    if (it == nullptr) {
        std::cout << "it == nullptr" << std::endl;
        return;
    }
    std::cout << "set:" << std::endl;
    while (it->isValid()){
           IVector* vec = nullptr;
           it->getVectorCopy(vec);
           printVector(vec);
           it->previous();
           delete vec;
    }
    delete it;
}


void testInsert(ISet* const& set, size_t _dim, double const* const& data){
    auto vec = IVector::createVector(_dim, data);
    std::cout << "insert ";
    printVector(vec);
    RC rc = set->insert(vec, IVector::NORM::SECOND, epsilon);
    std::cout << "RC\t" << static_cast<int>(rc) << std::endl;
    delete vec;
}

void testIntersection(ISet const* set1, ISet const* set2){

    std::cout << "intersection:" << std::endl;
    auto intersection = ISet::makeIntersection(set1, set2, IVector::NORM::SECOND, epsilon);
    printSet(intersection);
    delete intersection;
}

void testUnion(ISet const* set1, ISet const* set2){
    std::cout << "union: " << std::endl;
    auto setUnion = ISet::makeUnion(set1, set2, IVector::NORM::SECOND, epsilon);
    printSet(setUnion);
    delete setUnion;
}

void testSub(ISet const* set1, ISet const* set2){
    std::cout << "sub: " << std::endl;
    auto setSub = ISet::sub(set1, set2, IVector::NORM::SECOND, epsilon);
    printSet(setSub);
    delete setSub;
}

void testSymSub(ISet const* set1, ISet const* set2){
    std::cout << "symsub: " << std::endl;
    auto symSub = ISet::symSub(set1, set2, IVector::NORM::SECOND, epsilon);
    printSet(symSub);
    delete symSub;
}

void testEquals(ISet const* set1, ISet const* set2){
    std::cout << "equals" << std::endl;
    printSet(set1);
    printSet(set2);
    std::cout << ISet::equals(set1, set2, IVector::NORM::SECOND, epsilon);
}

void testSubSet(ISet const* set1, ISet const * set2){
    std::cout << "subset" << std::endl;
    printSet(set1);
    printSet(set2);
    std::cout << ISet::subSet(set1, set2, IVector::NORM::SECOND, epsilon);
}

void testIterators(ISet const* const& set, ISet::IIterator* (ISet::*_getBegin)() const, RC (ISet::IIterator::*_next)(size_t)){
    if (set == nullptr){
        std::cout << "set == nullptr" << std::endl;
        return;
    }
    auto it = (set->*_getBegin)();
    if (it == nullptr){
        std::cout << "it == nullptr" << std::endl;
        return;
    }
    while (it->isValid()){
        IVector* vec;
        RC getVectorRC = it->getVectorCopy(vec);
        if (getVectorRC != RC::SUCCESS){
            std::cout << "getVectorCopy return " << static_cast<int>(getVectorRC) << std::endl;
            delete vec;
            delete it;
            return;
        }
        printVector(vec);
        delete vec;
        vec = nullptr;
        RC nextItRC = (it->*_next)(1);
        if (nextItRC != RC::SUCCESS && nextItRC != RC::INDEX_OUT_OF_BOUND){
            std::cout << "ISet::Iterator::next return " << static_cast<int>(nextItRC) << std::endl;;
            delete vec;
            delete it;
            return;
        }
        delete vec;
        vec = nullptr;
    }
    std::cout << "test passed" << std::endl << std::endl;
    delete it;
}

void testIterators(ISet const* const& set){
    testIterators(set, &ISet::getBegin, &ISet::IIterator::next);
    testIterators(set, &ISet::getEnd, &ISet::IIterator::previous);
}


void testISet(){
    auto set1 = ISet::createSet();
    auto set2 = ISet::createSet();
    std::cout << "fill set1:" << std::endl;
    size_t i = 0;
    for (i = 0; i < 4; i++)
        testInsert(set1, dim, vectors[i]);
    std::cout << "fill set2" << std::endl;
    for (i = 2; i < vectors.size(); i++)
        testInsert(set2, dim, vectors[i]);


    testIterators(set1);

    testIntersection(set1, set2);

    testUnion(set1, set2);

    testSub(set1, set2);

    testSymSub(set1, set2);

    testEquals(set1, set2);

    testSubSet(set1, set1);

    delete set1;
    delete set2;
}


namespace comp {
    double const
    e11[] = {1, 1},
    e01[] = {-1, 1},
    e00[] = {-1, -1},
    e10[] = {1, -1},
    e0[]   = {0, 0, 0},
    e12[]  = {1./2, 1./2},
    e22[]  = {-1./2, -1./2};

    size_t const dim = 2;
    size_t const gridArr[] = {1, 1};

    size_t const orderArr[] = {0, 1};

    void printCompact(ICompact const * const& com, IMultiIndex const * const& order){
        if (com == nullptr){
            std::cout << "comp in null" << std::endl;
            return;
        }
        auto it = com->getBegin(order);
        if (it == nullptr){
            std::cout << "it in null" << std::endl;
            return;
        }
        while (it->isValid()){
            IVector* vec;
            it->getVectorCopy(vec);
            if (vec == nullptr){
                std::cout << "vec is null" << std::endl;
                return;
            }
            printVector(vec);
            delete vec;
            vec = nullptr;
            it->next();
        }
        delete it;
        std::cout << std::endl;
    }



    void testICompact() {
        auto v11 = IVector::createVector(dim, e11);
        auto v01 = IVector::createVector(dim, e01);
        auto v00 = IVector::createVector(dim, e00);
        auto v10 = IVector::createVector(dim, e10);
        auto v0   = IVector::createVector(dim, e0);
        auto v12  = IVector::createVector(dim, e12);
        auto v22  = IVector::createVector(dim, e22);
        auto grid = IMultiIndex::createMultiIndex(dim, gridArr);
        auto order = IMultiIndex::createMultiIndex(dim, orderArr);

        auto com11 = ICompact::createCompact(v11, v0, grid);
        auto com01 = ICompact::createCompact(v01, v0, grid);
        auto com00 = ICompact::createCompact(v00, v0, grid);
        auto com10 = ICompact::createCompact(v10, v0, grid);
        auto com0 = ICompact::createCompact(v12, v22, grid);

        auto v2 = IVector::add(v11, v11);
        auto comRemote = ICompact::createCompact(v2, v11, grid);
        //printCompact(ICompact::createCompactSpan(com00, com0, grid), order);

        //printCompact(ICompact::createCompactSpan(com10, com01, grid), order);

        printCompact(comRemote, order);
        printCompact(com00, order);

        //printCompact(ICompact::createIntersection(com0, com11, grid, epsilon), order);
        auto resCom = ICompact::createIntersection(com00, comRemote, grid, 2.);
        printCompact(resCom, order);

        delete resCom;
        delete v11;
        delete v01;
        delete v00;
        delete v10;
        delete v0;
        delete v12;
        delete v22;
        delete grid;
        delete order;
        delete com0;
        delete com11;
        delete com01;
        delete com00;
        delete com10;
        delete v2;
        delete comRemote;
    }
}