#include "../include/ISet.h"
#include "../include/ISetControlBlock.h"
#include <cstring>
#include <memory>
#include <functional>
#include <utility>
#include <cmath>

#define SendLog(Logger, Code, Level) if (Logger != nullptr) Logger->log((Code), (Level), __FILE__, __func__, __LINE__)
#define SendSevere(Logger, Code) if (Logger != nullptr) Logger->severe((Code), __FILE__, __func__, __LINE__)
#define SendWarning(Logger, Code) if (Logger != nullptr) Logger->warning((Code), __FILE__, __func__, __LINE__)
#define SendInfo(Logger, Code) if (Logger != nullptr) Logger->info((Code), __FILE__, __func__, __LINE__)


namespace{
    class SetControlBlock : public ISetControlBlock{
    private:
        static ILogger * _logger;
        size_t const* const* _hashCodesPtr;
        double const* const* _dataPtr;
        ISet* _set;
        bool* _setIsValid;

        inline void vectorIsValid(IVector const* const& vec, RC& rc, const char* const& srcfile, const char* const& function, int line) const;

        static inline void indexIncIsValid(size_t indexInc, RC& rc, const char* const& srcfile, const char* const& function, int line) ;

    public:

        RC getNext(IVector *const &vec, size_t &index, size_t indexInc) const override;

        RC getPrevious(IVector *const &vec, size_t &index, size_t indexInc) const override;

        RC getBegin(IVector *const &vec, size_t &index) const override;

        RC getEnd(IVector *const &vec, size_t &index) const override;

        ~SetControlBlock() override;

        SetControlBlock(ISet* set, size_t const * const * const& hashCodesPtr, double const * const * const& dataPtr, bool* setIsValid);
    };

    ILogger* SetControlBlock::_logger = nullptr;
}

RC SetControlBlock::getNext(IVector *const &vec, size_t &index, size_t indexInc) const {
    if (!(*_setIsValid))
        return RC::SOURCE_SET_DESTROYED;
    if ( _set->getSize() == 0)
        return RC::SOURCE_SET_EMPTY;

    RC validArgumentsRC = RC::SUCCESS;
    vectorIsValid(vec, validArgumentsRC, __FILE__, __FUNCTION__ , __LINE__);
    indexIncIsValid(indexInc, validArgumentsRC, __FILE__, __FUNCTION__, __LINE__);
    if (validArgumentsRC != RC::SUCCESS)
        return validArgumentsRC;

    for (size_t i = 0; i < _set->getSize() ; i++)
        if (index < (*_hashCodesPtr)[i]){
            if (i + indexInc - 1 >= _set->getSize()){
                SendInfo(_logger, RC::INDEX_OUT_OF_BOUND);
                return RC::INDEX_OUT_OF_BOUND;
            }
            RC setDataRC = vec->setData(_set->getDim(), (*_dataPtr) + (i + indexInc - 1) * _set->getDim());
            if (setDataRC != RC::SUCCESS){
                SendInfo(_logger, setDataRC);
                return setDataRC;
            }
            index = (*_hashCodesPtr)[i + indexInc - 1];
            return RC::SUCCESS;
        }
    return RC::INDEX_OUT_OF_BOUND;
}

RC SetControlBlock::getPrevious(IVector *const &vec, size_t &index, size_t indexInc) const {
    if (!(*_setIsValid))
        return RC::SOURCE_SET_DESTROYED;
    if ( _set->getSize() == 0)
        return RC::SOURCE_SET_EMPTY;

    RC validArgumentsRC = RC::SUCCESS;
    vectorIsValid(vec, validArgumentsRC, __FILE__, __FUNCTION__ , __LINE__);
    indexIncIsValid(indexInc, validArgumentsRC, __FILE__, __FUNCTION__, __LINE__);
    if (validArgumentsRC != RC::SUCCESS)
        return validArgumentsRC;

    for (size_t i = 0; i < _set->getSize(); i++)
        if (index > (*_hashCodesPtr)[_set->getSize() - 1 - i]) {
            size_t newIndex = _set->getSize() - 1 - i;
            if (newIndex - 1 + indexInc >= _set->getSize()){
                SendInfo(_logger, RC::INDEX_OUT_OF_BOUND);
                return RC::INDEX_OUT_OF_BOUND;
            }
            RC setDataRC = vec->setData(_set->getDim(), (*_dataPtr) + (newIndex - 1 + indexInc) * _set->getDim());
            if (setDataRC != RC::SUCCESS){
                SendInfo(_logger, setDataRC);
                return setDataRC;
            }
            index = (*_hashCodesPtr)[newIndex - 1 + indexInc];
            return RC::SUCCESS;
        }
    return RC::INDEX_OUT_OF_BOUND;
}

RC SetControlBlock::getBegin(IVector *const &vec, size_t &index) const {
    if (!(*_setIsValid))
        return RC::SOURCE_SET_DESTROYED;
    if ( _set->getSize() == 0)
        return RC::SOURCE_SET_EMPTY;

    RC validArgumentsRC = RC::SUCCESS;
    vectorIsValid(vec, validArgumentsRC, __FILE__, __FUNCTION__ , __LINE__);
    if (validArgumentsRC != RC::SUCCESS)
        return validArgumentsRC;

    RC setDataRC = vec->setData(_set->getDim(), (*_dataPtr));
    if (setDataRC != RC::SUCCESS){
        SendInfo(_logger, setDataRC);
        return setDataRC;
    }
    index = (*_hashCodesPtr)[0];
    return RC::SUCCESS;
}

RC SetControlBlock::getEnd(IVector *const &vec, size_t &index) const {
    if (!(*_setIsValid))
        return RC::SOURCE_SET_DESTROYED;
    if ( _set->getSize() == 0)
        return RC::SOURCE_SET_EMPTY;

    RC validArgumentsRC = RC::SUCCESS;
    vectorIsValid(vec, validArgumentsRC, __FILE__, __FUNCTION__ , __LINE__);
    if (validArgumentsRC != RC::SUCCESS)
        return validArgumentsRC;

    RC setDataRC = vec->setData(_set->getDim(), (*_dataPtr) + (_set->getSize() - 1) * _set->getDim());
    if (setDataRC != RC::SUCCESS){
        SendInfo(_logger, setDataRC);
        return setDataRC;
    }
    index = (*_hashCodesPtr)[_set->getSize() - 1];
    return RC::SUCCESS;
}

SetControlBlock::~SetControlBlock(){
    delete [] _setIsValid;
}

SetControlBlock::SetControlBlock(ISet *set, const size_t *const *const &hashCodesPtr,
                                 const double *const *const &dataPtr, bool* setIsValid) :
        _set(set),
        _hashCodesPtr(hashCodesPtr),
        _dataPtr(dataPtr),
        _setIsValid(setIsValid)
{}

void SetControlBlock::vectorIsValid(const IVector *const &vec, RC &rc, const char *const &srcfile,
                                    const char *const &function, int line) const{
    if (vec == nullptr){
        rc = RC::NULLPTR_ERROR;
        if (_logger != nullptr)
            _logger->log(RC::NULLPTR_ERROR, ILogger::Level::INFO, srcfile, function, line);
        return;
    }
    if (vec->getDim() != _set->getDim()){
        rc = RC::MISMATCHING_DIMENSIONS;
        if (_logger != nullptr)
            _logger->log(RC::MISMATCHING_DIMENSIONS, ILogger::Level::INFO, srcfile, function, line);
    }
}

void SetControlBlock::indexIncIsValid(size_t indexInc, RC &rc, const char *const &srcfile, const char *const &function,
                                      int line) {
    if (indexInc == 0){
        rc = RC::INVALID_ARGUMENT;
        if (_logger != nullptr)
            _logger->log(RC::INVALID_ARGUMENT, ILogger::Level::INFO, srcfile, function, line);
    }
}


ISetControlBlock::~ISetControlBlock() = default;


namespace {
    class Iterator : public ISet::IIterator{
    private:
        static ILogger * _logger;

        double* _data;
        size_t _dim;
        size_t _hash;
        std::shared_ptr<ISetControlBlock> _setCB;

        Iterator(size_t dim, size_t hash, double const *const &data, std::shared_ptr<ISetControlBlock> setCB);

        inline RC moveIterator(std::function<RC(IVector* const&, size_t&)> const & moveTo);

    public:

        IIterator * getNext(size_t indexInc) const override;

        IIterator * getPrevious(size_t indexInc) const override;

        IIterator * clone() const override;

        RC next(size_t indexInc) override;

        RC previous(size_t indexInc) override;

        bool isValid() const override;

        RC makeBegin() override;

        RC makeEnd() override;

        RC getVectorCopy(IVector *& val) const override;

        RC getVectorCoords(IVector * const& val) const override;

        ~Iterator() override;

        static Iterator *createIterator(size_t dim, double const *const &data, size_t hash, std::shared_ptr<ISetControlBlock> setCB);
    };

    ILogger* Iterator::_logger = nullptr;
}

ISet::IIterator *Iterator::getNext(size_t indexInc) const {
    auto it = clone();
    if (it == nullptr){
        SendInfo(_logger, RC::NULLPTR_ERROR);
        return nullptr;
    }
    RC nextRC = it->next(indexInc);
    if (nextRC != RC::SUCCESS){
        SendInfo(_logger, nextRC);
        delete it;
        return nullptr;
    }
    return it;
}

ISet::IIterator *Iterator::getPrevious(size_t indexInc) const {
    auto it = clone();
    if (it == nullptr){
        SendInfo(_logger, RC::NULLPTR_ERROR);
        return nullptr;
    }
    RC nextRC = it->previous(indexInc);
    if (nextRC != RC::SUCCESS){
        SendInfo(_logger, nextRC);
        delete it;
        return nullptr;
    }
    return it;
}

ISet::IIterator *Iterator::clone() const {
    if (_data == nullptr)
        return nullptr;
    auto it = new(std::nothrow) Iterator(_dim, _hash, _data, _setCB);
    if (it == nullptr)
        SendInfo(_logger, RC::ALLOCATION_ERROR);
    return it;
}

RC Iterator::moveIterator(std::function<RC(IVector* const&, size_t&)> const & moveTo){
    if (_data == nullptr){
        SendInfo(_logger, RC::INDEX_OUT_OF_BOUND);
        return RC::INDEX_OUT_OF_BOUND;
    }
    size_t hash = _hash;
    IVector* vec = IVector::createVector(_dim, _data);
    if (vec == nullptr){
        SendInfo(_logger, RC::NULLPTR_ERROR);
        return RC::NULLPTR_ERROR;
    }

    RC getNextRC = moveTo(vec, hash);
    if (getNextRC == RC::SOURCE_SET_EMPTY ||
        getNextRC == RC::SOURCE_SET_DESTROYED ||
        getNextRC == RC::INDEX_OUT_OF_BOUND){
        delete vec;
        delete [] _data;
        _data = nullptr;
        return getNextRC;
    }
    if (getNextRC != RC::SUCCESS){
        delete vec;
        SendInfo(_logger, getNextRC);
        return getNextRC;
    }
    _hash = hash;
    std::memcpy(_data, vec->getData(), _dim * sizeof(double));
    delete vec;

    return RC::SUCCESS;
}

RC Iterator::next(size_t indexInc) {
    auto moveTo = std::bind(&ISetControlBlock::getNext, _setCB.get(), std::placeholders::_1, std::placeholders::_2, indexInc);
    return moveIterator(moveTo);
}

RC Iterator::previous(size_t indexInc) {
    auto moveTo = std::bind(&ISetControlBlock::getPrevious, _setCB.get(), std::placeholders::_1, std::placeholders::_2, indexInc);
    return moveIterator(moveTo);
}

bool Iterator::isValid() const {
    return nullptr != _data;
}

RC Iterator::makeBegin() {
    auto moveTo = std::bind(&ISetControlBlock::getBegin, _setCB.get(), std::placeholders::_1, std::placeholders::_2);
    return moveIterator(moveTo);
}

RC Iterator::makeEnd() {
    auto moveTo = std::bind(&ISetControlBlock::getEnd, _setCB.get(), std::placeholders::_1, std::placeholders::_2);
    return moveIterator(moveTo);
}

RC Iterator::getVectorCopy(IVector *&val) const {
    auto vec = IVector::createVector(_dim, _data);
    if (vec == nullptr){
        SendInfo(_logger, RC::UNKNOWN);
        return RC::UNKNOWN;
    }
    val = vec;
    return RC::SUCCESS;
}

RC Iterator::getVectorCoords(IVector *const &val) const {
    if (val == nullptr){
        SendInfo(_logger, RC::NULLPTR_ERROR);
        return RC::NULLPTR_ERROR;
    }
    return val->setData(_dim, _data);
}

Iterator::~Iterator() {
    delete _data;
}

Iterator::Iterator(size_t dim, size_t hash, double const *const &data, std::shared_ptr<ISetControlBlock> setCB) :
        _dim(dim),
        _hash(hash),
        _setCB(std::move(setCB))
{
    if (dim == 0){
        SendInfo(_logger, RC::MISMATCHING_DIMENSIONS);
        return;
    }
    _data = new(std::nothrow) double[dim];
    if (_data == nullptr) {
        SendInfo(_logger, RC::ALLOCATION_ERROR);
        return;
    }
    std::memcpy(_data, data, dim * sizeof(double));
}

Iterator *Iterator::createIterator(size_t dim, double const *const &data, size_t hash,
                                   std::shared_ptr<ISetControlBlock> setCB) {
    if (data == nullptr || dim == 0){
        return nullptr;
    }
    auto* it = new(std::nothrow) Iterator(dim, hash, data, std::move(setCB));
    if (it == nullptr){
        SendInfo(_logger, RC::ALLOCATION_ERROR);
        return nullptr;
    }
    return it;
}


ISet::IIterator::~IIterator() = default;

RC ISet::IIterator::setLogger(ILogger *const pLogger) {
    return RC::INDEX_OUT_OF_BOUND;
}



namespace
{

    class Set : public ISet
    {
    private:
        size_t _dim;
        size_t _size;
        size_t _capacity;
        double* _data;
        size_t* _hashCodes;
        size_t _nextHash;
        std::shared_ptr<ISetControlBlock> _setCB;
        bool* _setIsValid;

    public:

        Set();

        ~Set() override;

        ISet* clone() const override;

        size_t getDim() const override;

        size_t getSize() const override;

        RC getCopy(size_t index, IVector *& val) const override;

        RC findFirstAndCopy(IVector const * const& pat, IVector::NORM n, double tol, IVector *& val) const override;

        RC getCoords(size_t index, IVector * const& val) const override;

        RC findFirstAndCopyCoords(IVector const * const& pat, IVector::NORM n, double tol, IVector * const& val) const override;

        RC findFirst(IVector const * const& pat, IVector::NORM n, double tol) const override;

        RC insert(IVector const * const& val, IVector::NORM n, double tol) override;

        RC remove(size_t index) override;

        RC remove(IVector const * const& pat, IVector::NORM n, double tol) override;

        IIterator *getIterator(size_t index) const override;

        IIterator *getBegin() const override;

        IIterator *getEnd() const override;

        static RC setLogger(ILogger* const);

        inline static void log(RC code, ILogger::Level level, const char* const& srcfile, const char* const& function, int line);

        inline void vectorIsValid(IVector const* const& vec, RC& rc, const char* const& srcfile, const char* const& function, int line) const;

        inline void indexIsValid(size_t index, RC& rc, const char* const& srcfile, const char* const& function, int line) const;

        static ILogger* _logger;
    };

    ILogger* Set::_logger = nullptr;
    size_t const capacityGain = 2;
    size_t const startCapacity = 2;
}

LIB_EXPORT RC ISet::setLogger(ILogger *const logger) {
    return Set::setLogger(logger);
}

LIB_EXPORT ISet *ISet::createSet() {
    return new(std::nothrow) Set();
}

LIB_EXPORT ISet *ISet::makeIntersection(const ISet *const &op1, const ISet *const &op2, IVector::NORM n, double tol) {
    if (op1 == nullptr || op2 == nullptr){
        SendInfo(Set::_logger, RC::NULLPTR_ERROR);
        return nullptr;
    }
    if (std::isnan(tol) || std::isinf(tol) || tol <= 0.){
        SendInfo(Set::_logger, RC::INVALID_ARGUMENT);
        return nullptr;
    }
    if (op1->getSize() == 0)
        return op1->clone();
    if (op2->getSize() == 0)
        return op2->clone();
    if (op1->getDim() != op2->getDim()){
        SendInfo(Set::_logger, RC::MISMATCHING_DIMENSIONS);
        return nullptr;
    }

    auto setRes = ISet::createSet();
    if (setRes == nullptr){
        SendInfo(Set::_logger, RC::NULLPTR_ERROR);
        return nullptr;
    }
    auto it = op1->getBegin();
    if (it == nullptr){
        delete setRes;
        SendInfo(Set::_logger, RC::NULLPTR_ERROR);
        return nullptr;
    }
    IVector* vec;
    RC getVectorRC = it->getVectorCopy(vec);
    auto checkError = [it, setRes, vec](bool cond){
        if (cond){
            delete it;
            delete vec;
            delete setRes;
            return true;
        }
        return false;
    };
    if (checkError(getVectorRC != RC::SUCCESS)){
        SendInfo(Set::_logger, getVectorRC);
        return nullptr;
    }
    do{
        RC foundVectorRC = op2->findFirst(vec, n, tol);
        if (foundVectorRC == RC::SUCCESS) {
            RC insertVectorRC = setRes->insert(vec, n, tol);
            if (checkError(insertVectorRC != RC::SUCCESS)) {
                SendInfo(Set::_logger, insertVectorRC);
                return nullptr;
            }
        }
        RC nextItRC = it->next();
        if (checkError(nextItRC != RC::SUCCESS && nextItRC != RC::INDEX_OUT_OF_BOUND)){
            SendInfo(Set::_logger, nextItRC);
            return nullptr;
        }
        if (!it->isValid())
            break;
        RC setVector = it->getVectorCoords(vec);
        if (checkError(setVector != RC::SUCCESS)){
            SendInfo(Set::_logger, setVector);
            return nullptr;
        }
    }while(true);
    delete vec;
    delete it;

    return setRes;
}

LIB_EXPORT ISet *ISet::makeUnion(const ISet *const &op1, const ISet *const &op2, IVector::NORM n, double tol) {
    if (op1 == nullptr || op2 == nullptr){
        SendInfo(Set::_logger, RC::NULLPTR_ERROR);
        return nullptr;
    }
    if (std::isnan(tol) || std::isinf(tol) || tol <= 0.){
        SendInfo(Set::_logger, RC::INVALID_ARGUMENT);
        return nullptr;
    }
    if (op1->getSize() == 0)
        return op2->clone();
    if (op2->getSize() == 0)
        return op1->clone();
    if (op1->getDim() != op2->getDim()){
        SendInfo(Set::_logger, RC::MISMATCHING_DIMENSIONS);
        return nullptr;
    }
    auto setRes = op1->clone();
    if (setRes == nullptr){
        SendInfo(Set::_logger, RC::NULLPTR_ERROR);
        return nullptr;
    }
    auto it = op2->getBegin();
    if (it == nullptr){
        delete setRes;
        SendInfo(Set::_logger, RC::NULLPTR_ERROR);
        return nullptr;
    }
    IVector* vec = nullptr;
    RC setVectorRC = it->getVectorCopy(vec);
    auto release = [setRes, it, vec](){
        delete setRes;
        delete it;
        delete vec;
    };
    if (setVectorRC != RC::SUCCESS){
        release();
        SendInfo(Set::_logger, setVectorRC);
        return nullptr;
    }
    do{
        RC insertVecRC = setRes->insert(vec, n, tol);
        if (insertVecRC != RC::SUCCESS && insertVecRC != RC::VECTOR_ALREADY_EXIST){
            release();
            SendInfo(Set::_logger, insertVecRC);
            return nullptr;
        }
        RC getNextItRC = it->next();
        if (getNextItRC != RC::SUCCESS && getNextItRC != RC::INDEX_OUT_OF_BOUND){
            release();
            SendInfo(Set::_logger, getNextItRC);
            return nullptr;
        }
        if (!it->isValid())
            break;
        RC copyVectorRC = it->getVectorCoords(vec);
        if (copyVectorRC != RC::SUCCESS){
            release();
            SendInfo(Set::_logger, copyVectorRC);
            return nullptr;
        }
    } while(true);
    delete it;
    delete vec;
    return setRes;
}

LIB_EXPORT ISet *ISet::sub(const ISet *const &op1, const ISet *const &op2, IVector::NORM n, double tol) {
    if (op1 == nullptr || op2 == nullptr){
        SendInfo(Set::_logger, RC::NULLPTR_ERROR);
        return nullptr;
    }
    if (std::isnan(tol) || std::isinf(tol) || tol <= 0.){
        SendInfo(Set::_logger, RC::INVALID_ARGUMENT);
        return nullptr;
    }
    if (op1->getSize() == 0)
        return op2->clone();
    if (op2->getSize() == 0)
        return op1->clone();
    if (op1->getDim() != op2->getDim()){
        SendInfo(Set::_logger, RC::MISMATCHING_DIMENSIONS);
        return nullptr;
    }
    auto setRes = op1->clone();
    if (setRes == nullptr){
        SendInfo(Set::_logger, RC::NULLPTR_ERROR);
        return nullptr;
    }
    auto it = op2->getBegin();
    if (it == nullptr){
        delete setRes;
        SendInfo(Set::_logger, RC::NULLPTR_ERROR);
        return nullptr;
    }
    IVector* vec = nullptr;
    RC setVectorRC = it->getVectorCopy(vec);
    auto release = [setRes, it, vec](){
        delete setRes;
        delete it;
        delete vec;
    };
    if (setVectorRC != RC::SUCCESS){
        release();
        SendInfo(Set::_logger, setVectorRC);
        return nullptr;
    }
    do{
        RC insertVecRC = setRes->remove(vec, n, tol);
        if (insertVecRC != RC::SUCCESS && insertVecRC != RC::VECTOR_NOT_FOUND){
            release();
            SendInfo(Set::_logger, insertVecRC);
            return nullptr;
        }
        RC getNextItRC = it->next();
        if (getNextItRC != RC::SUCCESS && getNextItRC != RC::INDEX_OUT_OF_BOUND){
            release();
            SendInfo(Set::_logger, getNextItRC);
            return nullptr;
        }
        if (!it->isValid())
            break;
        RC copyVectorRC = it->getVectorCoords(vec);
        if (copyVectorRC != RC::SUCCESS){
            release();
            SendInfo(Set::_logger, copyVectorRC);
            return nullptr;
        }
    } while(true);
    delete it;
    delete vec;
    return setRes;}

LIB_EXPORT ISet *ISet::symSub(const ISet *const &op1, const ISet *const &op2, IVector::NORM n, double tol) {
    if (op1 == nullptr || op2 == nullptr){
        SendInfo(Set::_logger, RC::NULLPTR_ERROR);
        return nullptr;
    }
    if (std::isnan(tol) || std::isinf(tol) || tol <= 0.){
        SendInfo(Set::_logger, RC::INVALID_ARGUMENT);
        return nullptr;
    }
    if (op1->getSize() == 0)
        return op2->clone();
    if (op2->getSize() == 0)
        return op1->clone();
    if (op1->getDim() != op2->getDim()){
        SendInfo(Set::_logger, RC::MISMATCHING_DIMENSIONS);
        return nullptr;
    }
    auto setUnion = ISet::makeUnion(op1, op2, n, tol);
    auto setInters = ISet::makeIntersection(op1, op2, n, tol);
    auto symSub = ISet::sub(setUnion, setInters, n, tol);
    delete setInters;
    delete setUnion;
    return symSub;
}

LIB_EXPORT bool ISet::equals(const ISet *const &op1, const ISet *const &op2, IVector::NORM n, double tol) {
    if (op1 == nullptr || op2 == nullptr){
        SendInfo(Set::_logger, RC::NULLPTR_ERROR);
        return false;
    }
    if (std::isnan(tol) || std::isinf(tol) || tol <= 0.){
        SendInfo(Set::_logger, RC::INVALID_ARGUMENT);
        return false;
    }
    if (op1 == op2)
        return true;

    if (op1->getDim() != op2->getDim()){
        SendInfo(Set::_logger, RC::MISMATCHING_DIMENSIONS);
        return false;
    }
    if (op1->getSize() != op2->getSize())
        return false;

    if (op1->getSize() == 0)
        return true;

    auto it = op1->getBegin();
    if (it == nullptr){
        SendInfo(Set::_logger, RC::NULLPTR_ERROR);
        return false;
    }
    IVector* vec = nullptr;
    RC setVectorRC = it->getVectorCopy(vec);
    if (setVectorRC != RC::SUCCESS){
        delete it;
        delete vec;
        SendInfo(Set::_logger, setVectorRC);
        return false;
    }

    do{
        RC foundVectorRC = op2->findFirst(vec, n, tol);
        if (foundVectorRC == RC::VECTOR_NOT_FOUND){
            delete it;
            delete vec;
            return false;
        }
        if (foundVectorRC != RC::SUCCESS){
            delete it;
            delete vec;
            SendInfo(Set::_logger, foundVectorRC);
            return false;
        }
        RC nextItRC = it->next();
        if (nextItRC != RC::SUCCESS && nextItRC != RC::INDEX_OUT_OF_BOUND){
            delete it;
            delete vec;
            SendInfo(Set::_logger, nextItRC);
            return false;
        }
        if (!it->isValid())
            break;
        RC setVectorCordRC = it->getVectorCoords(vec);
        if (setVectorCordRC != RC::SUCCESS){
            delete it;
            delete vec;
            SendInfo(Set::_logger, setVectorCordRC);
            return false;
        }
    }while(true);

    delete it;
    delete vec;
    return true;
}

LIB_EXPORT bool ISet::subSet(const ISet *const &op1, const ISet *const &op2, IVector::NORM n, double tol) {
    if (op1 == nullptr || op2 == nullptr){
        SendInfo(Set::_logger, RC::NULLPTR_ERROR);
        return false;
    }
    if (std::isnan(tol) || std::isinf(tol) || tol <= 0.){
        SendInfo(Set::_logger, RC::INVALID_ARGUMENT);
        return false;
    }
    if (op1->getDim() != op2->getDim()){
        SendInfo(Set::_logger, RC::MISMATCHING_DIMENSIONS);
        return false;
    }
    if (op2->getSize() == 0)
        return true;
    if (op1->getDim() == 0)
        return false;
     auto it = op2->getBegin();
     if (it == nullptr){
         SendInfo(Set::_logger, RC::NULLPTR_ERROR);
         return false;
     }
     IVector* vec = nullptr;
     RC setVectorRC = it->getVectorCopy(vec);
     if (setVectorRC != RC::SUCCESS) {
         delete it;
         delete vec;
         SendInfo(Set::_logger, setVectorRC);
         return false;
     }

     do{
         RC foundVectorRC = op1->findFirst(vec, n, tol);
         if (foundVectorRC == RC::VECTOR_NOT_FOUND){
             delete it;
             delete vec;
             return false;
         }
         if (foundVectorRC != RC::SUCCESS){
             delete it;
             delete vec;
             SendInfo(Set::_logger, foundVectorRC);
             return false;
         }
         RC nextRC = it->next();
         if (nextRC != RC::SUCCESS && nextRC != RC::INDEX_OUT_OF_BOUND){
             delete it;
             delete vec;
             SendInfo(Set::_logger, nextRC);
             return false;
         }
         if (!it->isValid())
             break;
         RC setVectorCordRC = it->getVectorCoords(vec);
         if (setVectorCordRC != RC::SUCCESS){
             delete it;
             delete vec;
             SendInfo(Set::_logger, setVectorCordRC);
             return false;
         }
     }while(true);

     delete it;
     delete vec;
    return true;
}

ISet::~ISet() = default;

Set::Set() :
        _size(0),
        _dim(0),
        _capacity(0),
        _data(nullptr),
        _hashCodes(nullptr),
        _nextHash(0){
    _setIsValid = new(std::nothrow) bool[1]{true};
    _setCB = std::shared_ptr<ISetControlBlock>(new(std::nothrow) SetControlBlock(this, &_hashCodes, &_data, _setIsValid));
}

Set::~Set() {
    delete [] _data;
    delete [] _hashCodes;
    _setIsValid[0] = false;
}

size_t Set::getDim() const {
    return _dim;
}

size_t Set::getSize() const {
    return _size;
}

RC Set::getCoords(size_t index, IVector *const &val) const {
    if (_size == 0)
        return RC::VECTOR_NOT_FOUND;
    RC rc = RC::SUCCESS;
    vectorIsValid(val, rc, __FILE__, __FUNCTION__, __LINE__);
    indexIsValid(index, rc, __FILE__, __FUNCTION__ , __LINE__);
    if (rc != RC::SUCCESS)
        return rc;

    return val->setData(_dim, _data + index * _dim);
}

RC Set::findFirstAndCopyCoords(const IVector *const &pat, IVector::NORM n, double tol, IVector *const &val) const {
    RC validVectorRC = RC::SUCCESS;
    vectorIsValid(pat, validVectorRC, __FILE__, __FUNCTION__ , __LINE__);
    vectorIsValid(val, validVectorRC, __FILE__, __FUNCTION__ , __LINE__);
    if (validVectorRC != RC::SUCCESS)
        return validVectorRC;

    if (_size == 0)
        return RC::VECTOR_NOT_FOUND;

    IVector* it = IVector::createVector(_dim, _data);
    if (it == nullptr) {
        Set::log(RC::NULLPTR_ERROR, ILogger::Level::INFO, __FILE__, __FUNCTION__ , __LINE__);
        return RC::NULLPTR_ERROR;
    }

    size_t i = 0;
    do{
        if (IVector::equals(it, pat, n, tol)) {
            RC setDataRC = val->setData(_dim, it->getData());
            delete it;
            if (setDataRC != RC::SUCCESS)
                Set::log(setDataRC, ILogger::Level::INFO, __FILE__, __FUNCTION__ , __LINE__);
            return setDataRC;
        }
        i++;
        if (i < _size){
            RC setDataRC = it->setData(_dim, _data + i * _dim);
            if (setDataRC != RC::SUCCESS){
                Set::log(setDataRC, ILogger::Level::INFO, __FILE__, __FUNCTION__ , __LINE__);
                delete it;
                return setDataRC;
            }
        }
    }while(i < _size);


    return RC::VECTOR_NOT_FOUND;
}

RC Set::insert(const IVector *const &val, IVector::NORM n, double tol) {
    if (_dim == 0 && val != nullptr) {
        _dim = val->getDim();
        _capacity = startCapacity;
        _data = new(std::nothrow) double[_capacity * _dim];
        _hashCodes = new(std::nothrow) size_t[_capacity];
        if (_data == nullptr || _hashCodes == nullptr){
            delete [] _data;
            delete [] _hashCodes;
            _data = nullptr;
            _hashCodes = nullptr;
            _capacity = 0;
            SendInfo(_logger, RC::NULLPTR_ERROR);
            return RC::NULLPTR_ERROR;
        }
    }
    RC checkValidVectorRC = RC::SUCCESS;
    vectorIsValid(val, checkValidVectorRC, __FILE__, __FUNCTION__ , __LINE__);
    if (checkValidVectorRC != RC::SUCCESS)
        return checkValidVectorRC;

    RC checkForVectorRC = findFirst(val, n, tol);
    if (checkForVectorRC == RC::SUCCESS)
        return RC::VECTOR_ALREADY_EXIST;
    if (checkForVectorRC != RC::VECTOR_NOT_FOUND) {
        Set::log(checkForVectorRC, ILogger::Level::INFO, __FILE__, __FUNCTION__, __LINE__);
        return checkForVectorRC;
    }

    double const* vecData = val->getData();
    if (vecData == nullptr){
        Set::log(RC::NULLPTR_ERROR, ILogger::Level::INFO, __FILE__, __FUNCTION__, __LINE__);
        return RC::NULLPTR_ERROR;
    }

    if (_size >= _capacity){
        auto* tmpData = new(std::nothrow) double[_capacity * capacityGain * _dim];
        auto* tmpHash = new(std::nothrow) size_t[_capacity * capacityGain];
        if (tmpHash == nullptr || tmpData == nullptr){
            delete [] tmpData;
            delete [] tmpHash;
            Set::log(RC::NULLPTR_ERROR, ILogger::Level::INFO, __FILE__, __FUNCTION__ , __LINE__);
            return RC::NULLPTR_ERROR;
        }
        std::memcpy(tmpData, _data, _capacity * _dim * sizeof(double));
        std::memcpy(tmpHash, _hashCodes, _capacity * sizeof(size_t));
        delete [] _data;
        delete [] _hashCodes;
        _data = tmpData;
        _hashCodes = tmpHash;

        _capacity *= capacityGain;
    }
    std::memcpy(_data + _size * _dim, vecData, _dim * sizeof (double));
    _hashCodes[_size] = _nextHash;
    _nextHash++;
    _size++;

    return RC::SUCCESS;
}

RC Set::remove(size_t index) {
    RC validIndexRC = RC::SUCCESS;
    indexIsValid(index, validIndexRC, __FILE__, __FUNCTION__ , __LINE__);
    if (validIndexRC != RC::SUCCESS)
        return validIndexRC;

    double * dest = _data + index * _dim;
    double * src = dest + _dim;
    size_t size = (_size - 1 - index) * _dim * sizeof(double);
    std::memmove(dest, src, size);
    std::memmove(_hashCodes + index, _hashCodes + index + 1, (_size - index - 1) * sizeof(size_t));
    _size--;

    return RC::SUCCESS;
}

RC Set::remove(const IVector *const &pat, IVector::NORM n, double tol) {
    RC validVectorRC = RC::SUCCESS;
    vectorIsValid(pat, validVectorRC, __FILE__, __FUNCTION__ , __LINE__);
    if (validVectorRC != RC::SUCCESS)
        return validVectorRC;

    if (_size == 0)
        return RC::VECTOR_NOT_FOUND;

    IVector* it = IVector::createVector(_dim, _data);
    if (it == nullptr){
        Set::log(RC::NULLPTR_ERROR, ILogger::Level::INFO, __FILE__, __FUNCTION__ , __LINE__);
        return RC::NULLPTR_ERROR;
    }
    size_t i = 0;
    do{
        if (IVector::equals(it, pat, n, tol)){
            RC removeRC = remove(i);
            if (removeRC != RC::SUCCESS)
                Set::log(removeRC, ILogger::Level::INFO, __FILE__, __FUNCTION__, __LINE__);
            delete it;
            return removeRC;
        }
        i++;
        if (i < _size){
            RC setDataRC = it->setData(_dim, _data + i * _dim);
            if (setDataRC != RC::SUCCESS){
                Set::log(setDataRC, ILogger::Level::INFO, __FILE__, __FUNCTION__ , __LINE__);
                delete it;
                return setDataRC;
            }
        }
    } while(i <= _size);

    delete it;
    return RC::VECTOR_NOT_FOUND;
}

RC Set::setLogger(ILogger *const logger) {
    if (logger == nullptr)
        return RC::NULLPTR_ERROR;
    Set::_logger = logger;
    return RC::SUCCESS;
}

RC Set::getCopy(size_t index, IVector *&val) const {
    RC argsIsValidRC = RC::SUCCESS;
    indexIsValid(index, argsIsValidRC, __FILE__, __FUNCTION__, __LINE__);
    if (argsIsValidRC != RC::SUCCESS)
        return argsIsValidRC;

    double* ptr_data = _data + index * _dim;
    val = IVector::createVector(_dim, ptr_data);
    if (val == nullptr) {
        log(RC::NULLPTR_ERROR, ILogger::Level::INFO, __FILE__, __FUNCTION__, __LINE__);
        return RC::NULLPTR_ERROR;
    }

    return RC::SUCCESS;
}

RC Set::findFirstAndCopy(const IVector *const &pat, IVector::NORM n, double tol, IVector *&val) const {
    RC argsIsValidRC = RC::SUCCESS;
    vectorIsValid(pat, argsIsValidRC, __FILE__, __FUNCTION__, __LINE__);
    if (argsIsValidRC != RC::SUCCESS)
        return argsIsValidRC;

    if (_size == 0)
        return RC::VECTOR_NOT_FOUND;

    IVector* it = IVector::createVector(_dim, _data);
    if (it == nullptr){
        Set::log(RC::NULLPTR_ERROR, ILogger::Level::INFO, __FILE__, __FUNCTION__ , __LINE__);
        return RC::NULLPTR_ERROR;
    }
    size_t i = 0;
    do{
        if (IVector::equals(it, pat, n, tol)){
            val = it;
            return RC::SUCCESS;
        }
        i++;
        if (i < _size){
            RC validCopyCordsRC = getCoords(i, it);
            if (validCopyCordsRC != RC::SUCCESS){
                delete it;
                log(validCopyCordsRC, ILogger::Level::INFO, __FILE__, __FUNCTION__ ,__LINE__);
                return validCopyCordsRC;
            }
        }
    }while(i <= _size);

    delete it;
    return RC::VECTOR_NOT_FOUND;
}

ISet *Set::clone() const {
    auto setClone = new(std::nothrow) Set();
    setClone->_data = new double[_capacity * _dim ];
    setClone->_hashCodes = new size_t[_capacity];
    std::memcpy(setClone->_data, _data, _size * _dim * sizeof (double));
    for (size_t hash = 0; hash < _size; hash++)
        setClone->_hashCodes[hash] = hash;
    setClone->_size = _size;
    setClone->_dim = _dim;
    setClone->_capacity = _capacity;
    setClone->_nextHash = _size;

    return setClone;
}

RC Set::findFirst(const IVector *const &pat, IVector::NORM n, double tol) const {
    RC validInputArgsRC = RC::SUCCESS;
    vectorIsValid(pat, validInputArgsRC, __FILE__, __FUNCTION__ , __LINE__);
    if (validInputArgsRC != RC::SUCCESS)
        return validInputArgsRC;

    if (_size == 0)
        return RC::VECTOR_NOT_FOUND;

    IVector* it = IVector::createVector(_dim, _data);
    size_t i = 0;
    if (it == nullptr) {
        log(RC::NULLPTR_ERROR, ILogger::Level::INFO, __FILE__, __FUNCTION__ , __LINE__);
        return RC::NULLPTR_ERROR;
    }

    do{
        if (IVector::equals(it, pat, n, tol)){
            delete it;
            return RC::SUCCESS;
        }
        i++;
        if (i < _size){
            RC setDataRC = it->setData(_dim, _data + i * _dim);
            if (setDataRC != RC::SUCCESS){
                delete it;
                log(setDataRC, ILogger::Level::INFO, __FILE__, __FUNCTION__ , __LINE__);
                return setDataRC;
            }
        }
    }while(i <= _size);

    delete it;
    return RC::VECTOR_NOT_FOUND;
}

ISet::IIterator *Set::getIterator(size_t index) const {
    if (_size <= index){
        SendInfo(_logger, RC::INDEX_OUT_OF_BOUND);
        return nullptr;
    }
    return Iterator::createIterator(_dim, _data + index * _dim, _hashCodes[index], _setCB);
}

ISet::IIterator *Set::getBegin() const {
    if (_size == 0){
        SendInfo(_logger, RC::SOURCE_SET_EMPTY);
        return nullptr;
    }
    return Iterator::createIterator(_dim, _data, _hashCodes[0], _setCB);
}

ISet::IIterator *Set::getEnd() const {
    if (_size == 0){
        SendInfo(_logger, RC::SOURCE_SET_EMPTY);
        return nullptr;
    }
    return Iterator::createIterator(_dim, _data + (_size - 1) * _dim, _hashCodes[_size - 1],_setCB);
}

inline void Set::log(RC code, ILogger::Level level, const char* const& srcfile, const char* const& function, int line)
{
    if (Set::_logger != nullptr)
        Set::_logger->log(code, level, srcfile, function, line);
}

void Set::vectorIsValid(const IVector *const &vec, RC &rc, const char* const& srcfile, const char* const& function, int line) const {
    if (vec == nullptr){
        Set::log(RC::NULLPTR_ERROR, ILogger::Level::INFO, srcfile, function, line);
        rc = RC::NULLPTR_ERROR;
    }
    if (vec->getDim() != _dim) {
        Set::log(RC::MISMATCHING_DIMENSIONS, ILogger::Level::INFO, srcfile, function, line);
        rc = RC::MISMATCHING_DIMENSIONS;
    }
}

void Set::indexIsValid(size_t index, RC &rc, const char* const& srcfile, const char* const& function, int line) const {
    if (index >= _size){
        Set::log(RC::INDEX_OUT_OF_BOUND, ILogger::Level::INFO, srcfile, function, line);
        rc = RC::INDEX_OUT_OF_BOUND;
    }
}


