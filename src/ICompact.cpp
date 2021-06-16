#include "../include/ICompact.h"
#include "../include/ICompactControlBlock.h"
#include <memory>
#include <cmath>
#include <cstring>

#define SendInfo(Logger, Code) if (Logger != nullptr) Logger->info((Code), __FILE__, __func__, __LINE__)


namespace {
    class MultiIndex : public IMultiIndex{
    private:
        size_t* _data;
        size_t _dim;

    public:

        static ILogger* _logger;

        MultiIndex(size_t dim, size_t *indices);

        IMultiIndex * clone() const override;

        size_t getDim() const override;

        const size_t *getData() const override;

        RC setData(size_t dim, size_t const* const& ptr_data) override;

        RC getAxisIndex(size_t axisIndex, size_t &val) const override;

        RC setAxisIndex(size_t axisIndex, size_t val) override;

        RC incAxisIndex(size_t axisIndex, ssize_t val) override;

        ~MultiIndex() override;
    };

    ILogger* MultiIndex::_logger = nullptr;
}

IMultiIndex* IMultiIndex::createMultiIndex(size_t dim, const size_t *indices) {
    if (indices == nullptr){
        SendInfo(MultiIndex::_logger, RC::NULLPTR_ERROR);
        return nullptr;
    }
    auto data = new (std::nothrow) size_t[dim];
    std::memcpy(data, indices, dim * sizeof(size_t));

    return new(std::nothrow) MultiIndex(dim, data);
}

RC IMultiIndex::setLogger(ILogger *const pLogger) {
    if (pLogger == nullptr){
        SendInfo(MultiIndex::_logger, RC::NULLPTR_ERROR);
        return RC::NULLPTR_ERROR;
    }
    MultiIndex::_logger = pLogger;
    return RC::SUCCESS;
}

ILogger* IMultiIndex::getLogger() {
    return MultiIndex::_logger;
}

MultiIndex::MultiIndex(size_t dim, size_t *indices) :
    _data(indices),
    _dim(dim){

}

IMultiIndex *MultiIndex::clone() const {
    return createMultiIndex(_dim, _data);
}

size_t MultiIndex::getDim() const {
    return _dim;
}

const size_t *MultiIndex::getData() const {
    return _data;
}

RC MultiIndex::setData(size_t dim, const size_t *const &ptr_data) {
    if (ptr_data == nullptr){
        SendInfo(_logger, RC::NULLPTR_ERROR);
        return RC::NULLPTR_ERROR;
    }
    if (_dim != dim){
        SendInfo(_logger, RC::MISMATCHING_DIMENSIONS);
        return RC::MISMATCHING_DIMENSIONS;
    }
    std::memcpy(_data, ptr_data, _dim * sizeof(size_t));
    return RC::SUCCESS;
}

RC MultiIndex::getAxisIndex(size_t axisIndex, size_t &val) const {
    if (axisIndex >= _dim){
        SendInfo(_logger, RC::MISMATCHING_DIMENSIONS);
        return RC::MISMATCHING_DIMENSIONS;
    }
    val = _data[axisIndex];

    return RC::SUCCESS;
}

RC MultiIndex::setAxisIndex(size_t axisIndex, size_t val) {
    if (axisIndex >= _dim){
        SendInfo(_logger, RC::MISMATCHING_DIMENSIONS);
        return RC::MISMATCHING_DIMENSIONS;
    }
    _data[axisIndex] = val;
    return RC::SUCCESS;
}

RC MultiIndex::incAxisIndex(size_t axisIndex, ssize_t val) {
    if (axisIndex >= _dim){
        SendInfo(_logger, RC::MISMATCHING_DIMENSIONS);
        return RC::MISMATCHING_DIMENSIONS;
    }

    _data[axisIndex] += val;
    return RC::SUCCESS;
}

MultiIndex::~MultiIndex() {
    delete _data;
}

IMultiIndex::~IMultiIndex() = default;


namespace {
    class CompactControlBlock : public ICompactControlBlock{
    private:
        ICompact* _compact;
        std::shared_ptr<bool> _isValid;

    public:

        CompactControlBlock(ICompact *compact, std::shared_ptr<bool> isValid);

        RC get(IMultiIndex * const &currentIndex, IMultiIndex const * const &bypassOrder) const override;

        RC get(IMultiIndex const * const &currentIndex, IVector* const &val) const override;

        ~CompactControlBlock() override;

    };
}

CompactControlBlock::CompactControlBlock(ICompact *compact, std::shared_ptr<bool> isValid) :
        _compact(compact),
        _isValid(std::move(isValid)){

}

CompactControlBlock::~CompactControlBlock() = default;

RC CompactControlBlock::get(IMultiIndex *const &currentIndex, const IMultiIndex *const &bypassOrder) const {
    if (currentIndex == nullptr || bypassOrder == nullptr)
        return RC::NULLPTR_ERROR;
    if (!(*_isValid))
        return RC::SOURCE_COMPACT_DESTROYED;
    if (_compact->getDim() != currentIndex->getDim() || _compact->getDim() != bypassOrder->getDim())
        return RC::MISMATCHING_DIMENSIONS;

    std::unique_ptr<IMultiIndex> grid(_compact->getGrid());
    if (grid == nullptr)
        return RC::NULLPTR_ERROR;

    size_t currentCoord, gridCoord;
    auto order = bypassOrder->getData();
    if (order == nullptr)
        return RC::NULLPTR_ERROR;

    size_t nextIncrement;
    for (nextIncrement = 0; nextIncrement < _compact->getDim(); nextIncrement++) {
        RC getCurCoordRC = currentIndex->getAxisIndex(order[nextIncrement], currentCoord);
        RC getGridCoordRC = grid->getAxisIndex(order[nextIncrement], gridCoord);
        if (getCurCoordRC != RC::SUCCESS || getGridCoordRC != RC::SUCCESS)
            return getCurCoordRC != RC::SUCCESS ? getCurCoordRC : getGridCoordRC;

        if (currentCoord == gridCoord) {
            RC setValueRC = currentIndex->setAxisIndex(order[nextIncrement], 0);
            if (setValueRC != RC::SUCCESS)
                return setValueRC;
            continue;
        }
        else if (currentCoord > gridCoord) {
            return RC::INDEX_OUT_OF_BOUND;
        }
        RC incValueRC = currentIndex->incAxisIndex(order[nextIncrement], 1);
        if (incValueRC != RC::SUCCESS){
            return incValueRC;
        }
        return RC::SUCCESS;
    }
    return RC::INDEX_OUT_OF_BOUND;
}

RC CompactControlBlock::get(const IMultiIndex *const &currentIndex, IVector *const &val) const {
    if (currentIndex == nullptr || val == nullptr)
        return RC::NULLPTR_ERROR;
    if (!(*_isValid))
        return RC::SOURCE_COMPACT_DESTROYED;

    return _compact->getVectorCoords(currentIndex, val);
}

ICompactControlBlock::~ICompactControlBlock() = default;

namespace {
    class CompactIterator : public ICompact::IIterator{
    private:
        std::shared_ptr<ICompactControlBlock> _cb;
        IMultiIndex* _currentIndex;
        IMultiIndex* _order;

    public:

        static ILogger* _logger;

        CompactIterator(IMultiIndex* currentIndex, IMultiIndex* order, std::shared_ptr<ICompactControlBlock> cb);

        IIterator * getNext() override;

        IIterator * clone() const override;

        RC next() override;

        bool isValid() const override;

        RC getVectorCopy(IVector *& val) const override;

        RC getVectorCoords(IVector * const& val) const override;

        ~CompactIterator() override;
    };

    ILogger* CompactIterator::_logger = nullptr;
}

RC ICompact::IIterator::setLogger(ILogger *const pLogger) {
    if (pLogger == nullptr){
        SendInfo(CompactIterator::_logger, RC::NULLPTR_ERROR);
        return RC::NULLPTR_ERROR;
    }
    CompactIterator::_logger = pLogger;
    return RC::SUCCESS;
}

ILogger* ICompact::IIterator::getLogger() {
    return CompactIterator::_logger;
}

ICompact::IIterator *CompactIterator::getNext() {
    auto newIt = clone();
    if (newIt == nullptr){
        SendInfo(_logger, RC::NULLPTR_ERROR);
        return nullptr;
    }
    RC moveItRC = newIt->next();
    if (moveItRC != RC::SUCCESS){
        SendInfo(_logger, moveItRC);
        return nullptr;
    }
    return newIt;
}

ICompact::IIterator *CompactIterator::clone() const {
    if (_currentIndex == nullptr)
        return nullptr;
    auto newIndex = _currentIndex->clone();
    auto newOrder = _order->clone();

    if (newIndex == nullptr || newOrder == nullptr){
        SendInfo(_logger, RC::NULLPTR_ERROR);
        delete newIndex;
        delete newOrder;
        return nullptr;
    }
    return new(std::nothrow) CompactIterator(newIndex, newOrder, _cb);
}

RC CompactIterator::next() {
    if (_currentIndex == nullptr)
        return RC::INDEX_OUT_OF_BOUND;

    RC moveIndexRC = _cb->get(_currentIndex, _order);
    if (moveIndexRC == RC::INDEX_OUT_OF_BOUND){
        delete _currentIndex;
        delete _order;
        _currentIndex = _order = nullptr;
    }
    return moveIndexRC;
}

bool CompactIterator::isValid() const {
    return _currentIndex != nullptr;
}

RC CompactIterator::getVectorCopy(IVector *&val) const {
    if (_currentIndex == nullptr)
        return RC::INDEX_OUT_OF_BOUND;
    auto data = new(std::nothrow) double[_currentIndex->getDim()];
    std::memset(data, 0, _currentIndex->getDim() * sizeof(double));
    if (data == nullptr){
        SendInfo(_logger, RC::ALLOCATION_ERROR);
        return RC::ALLOCATION_ERROR;
    }
    val = IVector::createVector(_currentIndex->getDim(), data);
    delete[] data;
    if (val == nullptr){
        SendInfo(_logger, RC::NULLPTR_ERROR);
        return RC::NULLPTR_ERROR;
    }

    return _cb->get(_currentIndex, val);
}

RC CompactIterator::getVectorCoords(IVector *const &val) const {
    if (_currentIndex == nullptr)
        return RC::INDEX_OUT_OF_BOUND;
    return _cb->get(_currentIndex, val);;
}

CompactIterator::~CompactIterator() {
    delete _currentIndex;
    delete _order;
}

ICompact::IIterator::~IIterator() = default;


CompactIterator::CompactIterator(IMultiIndex *currentIndex, IMultiIndex* order, std::shared_ptr<ICompactControlBlock> cb) :
                                 _currentIndex(currentIndex),
                                 _order(order),
                                 _cb(std::move(cb)){

}


namespace {
    class Compact : public ICompact{
    private:
        IMultiIndex* _grid;
        IVector* _lBorder;
        IVector* _rBorder;
        std::shared_ptr<bool> _compactIsValid;
        std::shared_ptr<ICompactControlBlock> _cb;

    public:

        Compact(IVector *const lBorder, IVector *const rBorder, IMultiIndex *grid);

        static ILogger* _logger;

        ICompact *clone() const override;

        bool isInside(IVector const * const&vec) const override;

        RC getVectorCopy(IMultiIndex const *index, IVector *& val) const override;

        RC getVectorCoords(IMultiIndex const *index, IVector * const& val) const override;

        RC getLeftBoundary(IVector *& vec) const override;

        RC getRightBoundary(IVector *& vec) const override;

        size_t getDim() const override;

        IMultiIndex* getGrid() const override;

        IIterator* getIterator(IMultiIndex const * const&index, IMultiIndex const * const &bypassOrder) const override;

        IIterator* getBegin(IMultiIndex const * const &bypassOrder) const override;

        IIterator* getEnd(IMultiIndex const * const &bypassOrder) const override;

        inline RC bypassOrderIsValid(IMultiIndex const * const& bypassOrder) const;

        ~Compact() override;

    };

    ILogger* Compact::_logger = nullptr;
}

ICompact* ICompact::createCompact(const IVector *vec1, const IVector *vec2, const IMultiIndex *nodeQuantities) {
    if (vec1 == nullptr || vec2 == nullptr || nodeQuantities == nullptr){
        SendInfo(Compact::_logger, RC::NULLPTR_ERROR);
        return nullptr;
    }
    if (vec1->getDim() != vec2->getDim() || vec1->getDim() != nodeQuantities->getDim()){
        SendInfo(Compact::_logger, RC::MISMATCHING_DIMENSIONS);
        return nullptr;
    }

    IVector* lBorder = vec1->clone();
    IVector* rBorder = vec2->clone();
    IMultiIndex* grid = nodeQuantities->clone();

    if (lBorder == nullptr || rBorder == nullptr || grid == nullptr){
        SendInfo(Compact::_logger, RC::NULLPTR_ERROR);
        delete lBorder;
        delete rBorder;
        delete grid;
        return nullptr;
    }

    double lVal, rVal;
    for (size_t i = 0; i < lBorder->getDim(); i++){
        RC getLValRC = lBorder->getCord(i, lVal);
        RC getRValRC = rBorder->getCord(i, rVal);
        if (getLValRC != RC::SUCCESS || getRValRC != RC::SUCCESS){
            SendInfo(Compact::_logger, (getLValRC != RC::SUCCESS ? getLValRC : getRValRC));
            delete lBorder;
            delete rBorder;
            delete grid;
            return nullptr;
        }

        if (lVal > rVal){
            RC setLValRC = lBorder->setCord(i, rVal);
            RC setRValRC = rBorder->setCord(i, lVal);
            if (setLValRC != RC::SUCCESS || setRValRC != RC::SUCCESS){
                SendInfo(Compact::_logger, (setLValRC != RC::SUCCESS ? setLValRC : setRValRC));
                delete lBorder;
                delete rBorder;
                delete grid;
                return nullptr;
            }
        }
    }
    return new(std::nothrow) Compact(lBorder, rBorder, grid);
}

/**
 *
 * @tparam VectorType  type must have method RC getDim(size_t, ElementType);
 * @tparam ElementType vector coordinates type
 * @param op1
 * @param op2
 * @param getter method of getting element by index
 * @param comparator
 * @param rc
 * @return if for all op1 and op2 coordinates comparator(elem1, elem2) == true, return true; else return false
 */
template <typename VectorType, typename ElementType>
inline bool VectorComparator(VectorType const* op1, VectorType const* op2,
                      RC (VectorType::* getter)(size_t, ElementType&) const,
                      std::function<bool(ElementType, ElementType)>const& comparator,
                      RC& rc){

    if (op1 == nullptr || op2 == nullptr){
        rc = RC::NULLPTR_ERROR;
        return false;
    }
    if (op1->getDim() != op2->getDim()){
        rc = RC::MISMATCHING_DIMENSIONS;
        return false;
    }
    ElementType lesserElem, largerElem; // mean that on the set of vector elements the order is given
    size_t dim = op1->getDim();
    for (size_t i = 0; i < dim; i++){
        RC getLesserRC = (op1->*getter)(i, lesserElem);
        RC getLargerRC = (op2->*getter)(i, largerElem);

        if (getLesserRC != RC::SUCCESS || getLargerRC != RC::SUCCESS){
            rc = getLesserRC != RC::SUCCESS ? getLesserRC : getLargerRC;
            return false;
        }
        if (!comparator(lesserElem, largerElem))
            return false;
    }
    return true;
}

inline bool vectorLE(IVector const * op1, IVector const * op2, RC& rc){
    std::function<bool(double, double)> le = [](double x, double y){ return x <= y; };
    return VectorComparator(op1, op2, &IVector::getCord, le, rc);
}

inline bool vectorME(IVector const* op1, IVector const* op2, RC& rc){
    std::function<bool(double, double)> me = [](double x, double y){ return x >= y; };
    return VectorComparator(op1, op2, &IVector::getCord, me, rc);
}

inline bool multiIndexME(IMultiIndex const* op1, IMultiIndex const* op2, RC& rc){
    std::function<bool(size_t, size_t)> le = [](size_t x, size_t y){ return x >= y; };
    return VectorComparator(op1, op2, &IMultiIndex::getAxisIndex, le, rc);
}

inline IVector* selectCoordinates(IVector const* op1, IVector const* op2, std::function<bool(double, double)> const& comparator, RC& rc){
    if (op1 == nullptr || op2 == nullptr){
        rc = RC::NULLPTR_ERROR;
        return nullptr;
    }
    if (op1->getDim() != op2->getDim()){
        rc = RC::MISMATCHING_DIMENSIONS;
        return nullptr;
    }
    auto result = op1->clone();
    if (result == nullptr){
        rc = RC::NULLPTR_ERROR;
        return nullptr;
    }

    size_t dim = result->getDim();
    double lesserElem, largerElem;
    for (size_t i = 0; i < dim; i++){
        RC getLesserRC = op1->getCord(i, lesserElem);
        RC getLargerRC = op2->getCord(i, largerElem);
        if (getLesserRC != RC::SUCCESS || getLargerRC != RC::SUCCESS){
            delete result;
            rc = getLesserRC != RC::SUCCESS ? getLesserRC : getLargerRC;
            return nullptr;
        }
        if (!comparator(lesserElem, largerElem)){
            RC setLesserRC = result->setCord(i, largerElem);
            if (setLesserRC != RC::SUCCESS){
                delete result;
                rc = setLesserRC;
                return nullptr;
            }
        }
    }
    return result;
}

inline IVector* selectLECoords(IVector const* op1, IVector const* op2, RC& rc){
    auto le = [](double x, double y){ return x <= y; };
    return selectCoordinates(op1, op2, le, rc);
}

inline IVector* selectMECoords(IVector const* op1, IVector const* op2, RC& rc){
    auto me = [](double x, double y){ return x >= y; };
    return selectCoordinates(op1, op2, me, rc);
}

inline IVector* extendLeftBounder(IVector const* lBounder, IVector const* rBounder, double tol, RC& rc){
    auto cond = [tol](double l, double r){ return  l <= r || std::abs(r - l) > tol; };
    return selectCoordinates(lBounder, rBounder, cond, rc);
}

inline IVector* extendRightBounder(IVector const* rBounder, IVector const* lBounder, double tol, RC& rc){
    auto cond = [tol](double r, double l){ return  l <= r || std::abs(r - l) > tol; };
    return selectCoordinates(rBounder, lBounder, cond, rc);
}

ICompact* ICompact::createCompactSpan(const ICompact *op1, const ICompact *op2, const IMultiIndex *const grid) {
    if (op1 == nullptr || op2 == nullptr || grid == nullptr){
        SendInfo(Compact::_logger, RC::NULLPTR_ERROR);
        return nullptr;
    }
    if (op1->getDim() != op2->getDim() || op1->getDim() != grid->getDim()){
        SendInfo(Compact::_logger, RC::MISMATCHING_DIMENSIONS);
        return nullptr;
    }
    IVector* vec1 = nullptr;
    IVector* vec2 = nullptr;

    RC getBoundary1RC = op1->getLeftBoundary(vec1);
    RC getBoundary2RC = op2->getLeftBoundary(vec2);
    if (getBoundary1RC != RC::SUCCESS || getBoundary2RC != RC::SUCCESS){
        SendInfo(Compact::_logger, getBoundary1RC != RC::SUCCESS ? getBoundary1RC : getBoundary2RC);
        delete vec1;
        delete vec2;
        return nullptr;
    }
    RC selectRightRC = RC::SUCCESS;
    auto leftBoundary = selectLECoords(vec1, vec2, selectRightRC);

    delete vec1;
    delete vec2;
    vec1 = vec2 = nullptr;
    getBoundary1RC = op1->getRightBoundary(vec1);
    getBoundary2RC = op2->getRightBoundary(vec2);
    if (getBoundary1RC != RC::SUCCESS || getBoundary2RC != RC::SUCCESS){
        SendInfo(Compact::_logger, getBoundary1RC != RC::SUCCESS ? getBoundary1RC : getBoundary2RC);
        delete vec1;
        delete vec2;
        return nullptr;
    }
    RC selectLeftRC = RC::SUCCESS;
    auto rightBoundary = selectMECoords(vec1, vec2, selectLeftRC);

    if (selectRightRC != RC::SUCCESS || selectLeftRC != RC::SUCCESS){
        SendInfo(Compact::_logger, selectRightRC != RC::SUCCESS ? selectLeftRC : selectRightRC);
        delete vec1;
        delete vec2;
        delete leftBoundary;
        delete rightBoundary;
        return nullptr;
    }
    delete vec1;
    delete vec2;
    auto newGrid = grid->clone();
    if (newGrid == nullptr){
        SendInfo(Compact::_logger, RC::NULLPTR_ERROR);
        delete leftBoundary;
        delete rightBoundary;
        return nullptr;
    }
    return new(std::nothrow) Compact(leftBoundary, rightBoundary, newGrid);
}

ICompact* ICompact::createIntersection(const ICompact *op1, const ICompact *op2, const IMultiIndex *const grid, double tol) {
    if (op1 == nullptr || op2 == nullptr || grid == nullptr){
        SendInfo(Compact::_logger, RC::NULLPTR_ERROR);
        return nullptr;
    }
    if (std::isnan(tol) || std::isinf(tol) || tol <= 0.){
        SendInfo(Compact::_logger, RC::INVALID_ARGUMENT);
        return nullptr;
    }
    if (op1->getDim() != op2->getDim() || op1->getDim() != grid->getDim()){
        SendInfo(Compact::_logger, RC::MISMATCHING_DIMENSIONS);
        return nullptr;
    }

    IVector* lb1Vec = nullptr;
    IVector* rb1Vec = nullptr;
    IVector* lb2Vec = nullptr;
    IVector* rb2Vec = nullptr;
    RC getLB1RC = op1->getLeftBoundary(lb1Vec);
    RC getRB1RC = op1->getRightBoundary(rb1Vec);
    RC getLB2RC = op2->getLeftBoundary(lb2Vec);
    RC getRB2RC = op2->getRightBoundary(rb2Vec);

    if (getLB1RC != RC::SUCCESS || getRB1RC != RC::SUCCESS ||
        getLB2RC != RC::SUCCESS || getRB2RC != RC::SUCCESS){
        if (getLB1RC != RC::SUCCESS)
            SendInfo(Compact::_logger, getLB1RC);
        if (getRB1RC != RC::SUCCESS)
            SendInfo(Compact::_logger, getRB1RC);
        if (getLB2RC != RC::SUCCESS)
            SendInfo(Compact::_logger, getLB2RC);
        if (getRB2RC != RC::SUCCESS)
            SendInfo(Compact::_logger, getRB2RC);

        delete lb1Vec;
        delete rb1Vec;
        delete lb2Vec;
        delete rb2Vec;
        return nullptr;
    }
    RC extendBounderRC = RC::SUCCESS;

    std::unique_ptr<IVector> extLeftBounder1(extendLeftBounder(lb1Vec, rb2Vec, tol, extendBounderRC));
    std::unique_ptr<IVector> extRightBounder1(extendRightBounder(rb1Vec, lb2Vec, tol, extendBounderRC));
    std::unique_ptr<IVector> extLeftBounder2(extendLeftBounder(lb2Vec, rb1Vec, tol, extendBounderRC));
    std::unique_ptr<IVector> extRightBounder2(extendRightBounder(rb2Vec, lb1Vec, tol, extendBounderRC));

    if (extendBounderRC != RC::SUCCESS){
        SendInfo(Compact::_logger, extendBounderRC);
        delete lb1Vec;
        delete rb1Vec;
        delete lb2Vec;
        delete rb2Vec;
        return nullptr;
    }
    RC selectNewBounderRC = RC::SUCCESS;
    auto newLeftBounder = selectMECoords(extLeftBounder1.get(), extLeftBounder2.get(), selectNewBounderRC);
    auto newRightBounder = selectLECoords(extRightBounder1.get(), extRightBounder2.get(), selectNewBounderRC);
    auto newGrid = grid->clone();

    if (selectNewBounderRC != RC::SUCCESS || newGrid == nullptr){
        if (selectNewBounderRC != RC::SUCCESS)
            SendInfo(Compact::_logger, selectNewBounderRC);
        if (newGrid == nullptr)
            SendInfo(Compact::_logger, RC::NULLPTR_ERROR);
        delete newGrid;
        delete lb1Vec;
        delete rb1Vec;
        delete lb2Vec;
        delete rb2Vec;
        return nullptr;
    }

    delete lb1Vec;
    delete rb1Vec;
    delete lb2Vec;
    delete rb2Vec;
    return new(std::nothrow) Compact(newLeftBounder, newRightBounder, newGrid);
}

RC ICompact::setLogger(ILogger *const logger) {
    if (logger == nullptr){
        SendInfo(Compact::_logger, RC::NULLPTR_ERROR);
        return RC::NULLPTR_ERROR;
    }
    Compact::_logger = logger;
    return RC::SUCCESS;
}

ILogger* ICompact::getLogger() {
    return Compact::_logger;
}

Compact::Compact(IVector *const lBorder, IVector *const rBorder, IMultiIndex *grid) :
    _lBorder(lBorder),
    _rBorder(rBorder),
    _grid(grid),
    _compactIsValid(new(std::nothrow)bool[1]{true}),
    _cb(new(std::nothrow) CompactControlBlock(this, _compactIsValid)){

}

ICompact *Compact::clone() const {
    auto lBounder = _lBorder->clone();
    auto rBounder = _rBorder->clone();
    auto grid = _grid->clone();
    if (lBounder == nullptr || rBounder == nullptr || grid == nullptr){
        SendInfo(_logger , RC::NULLPTR_ERROR);
        delete lBounder;
        delete rBounder;
        delete grid;
        return nullptr;
    }
    return new(std::nothrow) Compact(lBounder, rBounder, grid);
}

bool Compact::isInside(const IVector *const &vec) const {
    if (vec == nullptr){
        SendInfo(_logger, RC::NULLPTR_ERROR);
        return false;
    }
    if (vec->getDim() != _lBorder->getDim()){
        SendInfo(_logger, RC::MISMATCHING_DIMENSIONS);
        return false;
    }
    RC isInsideRC = RC::SUCCESS;
    if (vectorME(_lBorder, vec, isInsideRC) && vectorLE(vec, _rBorder, isInsideRC)){
        if (isInsideRC != RC::SUCCESS){
            SendInfo(_logger, isInsideRC);
            return false;
        }
        return true;
    }
    if (isInsideRC != RC::SUCCESS)
        SendInfo(_logger, isInsideRC);
    return false;
}

RC Compact::getVectorCopy(const IMultiIndex *index, IVector *&val) const {
    if (index == nullptr){
        SendInfo(_logger, RC::NULLPTR_ERROR);
        return RC::NULLPTR_ERROR;
    }
    auto newVector = _lBorder->clone();
    if (newVector == nullptr){
        SendInfo(_logger, RC::NULLPTR_ERROR);
        return RC::NULLPTR_ERROR;
    }
    RC getVectorCoordsRC = getVectorCoords(index, newVector);
    if (getVectorCoordsRC != RC::NULLPTR_ERROR){
        SendInfo(_logger, getVectorCoordsRC);
        delete newVector;
        return getVectorCoordsRC;
    }
    val = newVector;
    return RC::SUCCESS;
}

RC Compact::getVectorCoords(const IMultiIndex *index, IVector *const &val) const {
    if (index == nullptr || val == nullptr){
        SendInfo(_logger, RC::NULLPTR_ERROR);
        return RC::INVALID_ARGUMENT;
    }
    if (index->getDim() != _grid->getDim() || val->getDim() != _lBorder->getDim()){
        SendInfo(_logger, RC::MISMATCHING_DIMENSIONS);
        return RC::MISMATCHING_DIMENSIONS;
    }

    RC validGridRC = RC::SUCCESS;
    bool validGrid = multiIndexME(_grid, index, validGridRC);
    if (validGridRC != RC::SUCCESS){
        SendInfo(_logger, validGridRC);
        return validGridRC;
    }
    if (!validGrid){
        SendInfo(_logger, RC::INDEX_OUT_OF_BOUND);
        return RC::INDEX_OUT_OF_BOUND;
    }

    for (size_t i = 0; i < val->getDim(); i++){
        double leftVal, rightVal;
        size_t gridIndex, maxIndex;
        RC getLeftRC = _lBorder->getCord(i, leftVal);
        RC getRightRC = _rBorder->getCord(i, rightVal);
        RC getIndexRC = index->getAxisIndex(i, gridIndex);
        RC getMaxIndexRC = _grid->getAxisIndex(i, maxIndex);

        if (getIndexRC != RC::SUCCESS){
            SendInfo(_logger, getIndexRC);
            return getIndexRC;
        }
        if (getLeftRC != RC::SUCCESS){
            SendInfo(_logger, getLeftRC);
            return getLeftRC;
        }
        if (getRightRC != RC::SUCCESS){
            SendInfo(_logger, getRightRC);
            return getRightRC;
        }
        if (getMaxIndexRC != RC::SUCCESS){
            SendInfo(_logger, getMaxIndexRC);
            return getMaxIndexRC;
        }

        RC setValueRC = val->setCord(i, leftVal + (rightVal - leftVal) / static_cast<double>(maxIndex) * static_cast<double>(gridIndex));
        if (setValueRC != RC::SUCCESS){
            SendInfo(_logger, setValueRC);
            return setValueRC;
        }
    }

    return RC::SUCCESS;
}

RC Compact::getLeftBoundary(IVector *&vec) const {
    if (_lBorder == nullptr){
        SendInfo(_logger, RC::NULLPTR_ERROR);
        return RC::NULLPTR_ERROR;
    }
    vec = _lBorder->clone();
    if (vec == nullptr){
        SendInfo(_logger, RC::NULLPTR_ERROR);
        return RC::NULLPTR_ERROR;
    }
    return RC::SUCCESS;
}

RC Compact::getRightBoundary(IVector *&vec) const {
    if (_rBorder == nullptr){
        SendInfo(_logger, RC::NULLPTR_ERROR);
        return RC::NULLPTR_ERROR;
    }
    vec = _rBorder->clone();
    if (vec == nullptr){
        SendInfo(_logger, RC::NULLPTR_ERROR);
        return RC::NULLPTR_ERROR;
    }
    return RC::SUCCESS;
}

size_t Compact::getDim() const {
    if (_lBorder == nullptr){
        return 0;
    }
    return _lBorder->getDim();
}

IMultiIndex *Compact::getGrid() const {
    if (_grid == nullptr){
        SendInfo(_logger, RC::NULLPTR_ERROR);
        return nullptr;
    }
    return _grid->clone();
}

RC Compact::bypassOrderIsValid(const IMultiIndex *const &bypassOrder) const{
    if (bypassOrder == nullptr)
        return RC::NULLPTR_ERROR;
    if (bypassOrder->getDim() != getDim())
        return RC::MISMATCHING_DIMENSIONS;

    size_t dim = getDim();
    size_t indexVal;
    for (size_t requiredIndex = 0; requiredIndex < dim; requiredIndex++){
        for (size_t i = 0; i < dim; i++){
            RC getIndexRC = bypassOrder->getAxisIndex(i, indexVal);
            if (getIndexRC != RC::SUCCESS)
                return getIndexRC;

            if (indexVal == requiredIndex)
                break;
            else if (i == dim - 1)
                return RC::INVALID_ARGUMENT;
        }
    }
    return RC::SUCCESS;
}

ICompact::IIterator *Compact::getIterator(const IMultiIndex *const &index, const IMultiIndex *const &bypassOrder) const {
    if (index == nullptr || bypassOrder == nullptr){
        SendInfo(_logger, RC::NULLPTR_ERROR);
        return nullptr;
    }
    if (index->getDim() != _lBorder->getDim() || bypassOrder->getDim() != _lBorder->getDim()){
        SendInfo(_logger, RC::MISMATCHING_DIMENSIONS);
        return nullptr;
    }
    RC validGridRC = RC::SUCCESS;
    bool validGrid = multiIndexME(_grid, index, validGridRC);
    if (validGridRC != RC::SUCCESS){
        SendInfo(_logger, validGridRC);
        return nullptr;
    }
    if (!validGrid){
        SendInfo(_logger, RC::INDEX_OUT_OF_BOUND);
        return nullptr;
    }
    RC orderIsValidRC = bypassOrderIsValid(bypassOrder);
    if (orderIsValidRC != RC::SUCCESS){
        SendInfo(_logger, orderIsValidRC);
        return nullptr;
    }

    auto newIndex = index->clone();
    auto newOrder = bypassOrder->clone();
    if (newIndex== nullptr || newOrder == nullptr){
        SendInfo(_logger, RC::NULLPTR_ERROR);
        delete newOrder;
        delete newIndex;
        return nullptr;
    }

    return new(std::nothrow) CompactIterator(newIndex, newOrder, _cb);
}

ICompact::IIterator *Compact::getBegin(const IMultiIndex *const &bypassOrder) const {

    auto index = _grid->clone();
    for (size_t i = 0; i < _grid->getDim(); i++){
        RC setValRC = index->setAxisIndex(i, 0);
        if (setValRC != RC::SUCCESS){
            SendInfo(_logger, setValRC);
            delete index;
            return nullptr;
        }

    }
    auto result = getIterator(index, bypassOrder);
    delete index;

    if (result == nullptr)
        SendInfo(_logger, RC::NULLPTR_ERROR);

    return result;
}

ICompact::IIterator *Compact::getEnd(const IMultiIndex *const &bypassOrder) const {
    return getIterator(_grid, bypassOrder);
}

Compact::~Compact() {
    delete _lBorder;
    delete _rBorder;
    delete _grid;
    *_compactIsValid = false;
}

ICompact::~ICompact() = default;

