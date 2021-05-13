#include "../include/IVector.h"
#include <math.h>
#include <cstdint>
#include <new>
#include <cstring>


namespace
{

    class Vector : public IVector
    {
    private:
        static ILogger* logger;
        size_t _dim;

        RC adder(IVector const* const& op, double multiplier);


    public:
        Vector(size_t dim);
        ~Vector();

        inline double* getDataPointer();

        static double distFirstNorm(size_t dim, double const* dataOp1, double const* dataOp2 = nullptr);
        static double distSecondNorm(size_t dim, double const* dataOp1, double const* dataOp2 = nullptr);
        static double distInfinityNorm(size_t dim, double const* dataOp1, double const* dataOp2 = nullptr);
        static void log(RC code, ILogger::Level level, const char* const& srcfile, const char* const& function, int line);
///IAA: Вы хорошо тестировали?
        static IVector* applyOperator(IVector const* const& op1, IVector const* const& op2, std::function<double(double, double)> fun);

        IVector* clone() const override;
        double const* getData() const override;
        RC setData(size_t dim, double const* const& ptr_data) override;

        RC getCord(size_t index, double& val) const override;
        RC setCord(size_t index, double val) override;
        RC scale(double multiplier) override;
        size_t getDim() const override;
        double norm(NORM n) const override;

        RC inc(IVector const* const& op) override;
        RC dec(IVector const* const& op) override;

        RC applyFunction(const std::function<double(double)>& fun) override;
        RC foreach(const std::function<void(double)>& fun) const override;

        size_t sizeAllocated() const override;

        static RC setLoggerImpl(ILogger* const logger);

        ILogger* getLogger() const override;
    };

    ILogger* Vector::logger = nullptr;
};

Vector::Vector(size_t dim):
        _dim(dim)
{
}


/**
 * input:
 * size_t dim, double const* ptr_data, Logger* pLogger
 *
 * output:
 * IVector* - pointer to allocated initialized memory for Vector
 */
IVector* IVector::createVector(size_t dim, double const* const& ptr_data)
{
    if (dim == 0)
    {
        Vector::log(RC::MISMATCHING_DIMENSIONS, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return nullptr;
    }
    if (ptr_data == nullptr)
    {
        Vector::log(RC::NULLPTR_ERROR, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return nullptr;
    }
    for (size_t i = 0; i < dim; i++)
        if (isnan(ptr_data[i]) || isinf(ptr_data[i]))
        {
            Vector::log(RC::NULLPTR_ERROR, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
            return nullptr;
        }

    uint8_t* buffer = new(std::nothrow) uint8_t[dim * sizeof(double) + sizeof(Vector)];
    if (buffer == nullptr)
    {
        Vector::log(RC::ALLOCATION_ERROR, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return nullptr;
    }

    IVector* vec = new (buffer) Vector(dim);
    if (vec == nullptr)
    {
        Vector::log(RC::NULLPTR_ERROR, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        delete [] buffer;
        return nullptr;
    }

    auto* data = reinterpret_cast<double*>(reinterpret_cast<uint8_t*>(vec) + sizeof(Vector));
    std::memcpy(data, ptr_data, dim * sizeof(double));

    return vec;
}

Vector::~Vector()
{
///IAA: все верно, только переопределить operator delete не нужно
/**AAA:
    как я понял, для того чтобы успокоить статический анализатор
    нужно вызвать delete, комплиментарный вызванному new, чтобы удалить память также как мы её выделяли,
    для этого писать в деструктор ничего не нужно, а вот переопределить operator delete нужно.
 */

}

/**
* input:
* IVector const* const& op1,
* IVector const* const& op2,
* std::function<double(double, double)> fun - function that will apply to all coordinates
*
* output:
* IVector* - vector such what vector_i = fun(op1_i, op2_i) or nullptr
*/
IVector* Vector::applyOperator(IVector const* const& op1, IVector const* const& op2, std::function<double(double, double)> fun)
{
    if (op1 == nullptr || op2 == nullptr)
    {
        Vector::log(RC::NULLPTR_ERROR, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return nullptr;
    }
    if (op1->getDim() != op2->getDim())
    {
        Vector::log(RC::MISMATCHING_DIMENSIONS, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return nullptr;
    }
    size_t dim = op1->getDim();
    double const* dataOp1 = op1->getData();
    double const* dataOp2 = op2->getData();

    if (dataOp1 == nullptr || dataOp2 == nullptr)
    {
        Vector::log(RC::NULLPTR_ERROR, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return nullptr;
    }

    for (size_t i = 0; i < dim; i++)
    {
        double cordsSum = fun(dataOp1[i], dataOp2[i]);
        if (isnan(cordsSum))
        {
            Vector::log(RC::NOT_NUMBER, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
            return nullptr;
        }
        if (isinf(cordsSum))
        {
            Vector::log(RC::INFINITY_OVERFLOW, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
            return nullptr;
        }
    }

    IVector* newVec = op1->clone();
    if (newVec == nullptr)
    {
        Vector::log(RC::NULLPTR_ERROR, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return nullptr;
    }
    for (size_t i = 0; i < dim; i++)
    {
        RC rc = newVec->setCord(i, fun(dataOp1[i], dataOp2[i]));
        if (rc != RC::SUCCESS)
        {
            Vector::log(rc, ILogger::Level::INFO, __FILE__, __func__, __LINE__);\
            delete newVec;
            return nullptr;
        }
    }
    return newVec;
}

/**
 * inout:
 * IVector const* op1, IVector const* op2, Logger* pLogger
 *
 * output:
 * IVector* - pointer to new vector equal to sum op1 and op2
 * return nullptr if addition is not possible
 */
IVector* IVector::add(IVector const* const& op1, IVector const* const& op2)
{
    return Vector::applyOperator(op1, op2, [](double x, double y){return x + y;});
}

/**
 * input:
 * IVector const* op1, IVector const* op2, Logger* pLogger
 *
 * output:
 * IVector* - pointer to new vector equal to difference op1 and op2
 * return nullptr if difference is not possible
 */
IVector* IVector::sub(IVector const* const& op1, IVector const* const& op2)
{
    return Vector::applyOperator(op1, op2, [](double x, double y){return x - y;});
}

/**
 * input:
 * IVector const* op1, IVector const* op2, Logger* pLogger
 *
 * output:
 * double - dot product of vectors
 * return NAN if dot product is not possible
 */
double IVector::dot(IVector const* const& op1, IVector const* const& op2)
{
    if (op1 == nullptr || op2 == nullptr)
    {
        Vector::log(RC::NULLPTR_ERROR, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return NAN;
    }

    if (op1->getDim() != op2->getDim())
    {
        Vector::log(RC::MISMATCHING_DIMENSIONS, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return NAN;
    }
    size_t dim = op1->getDim();
    double const* dataOp1 = op1->getData();
    double const* dataOp2 = op2->getData();
    double _dot = 0.;
    for (size_t i = 0; i < dim; i++)
        _dot += dataOp1[i] * dataOp2[i];

    return _dot;
}

/**
 * input:
 * IVector const* op1, IVector const* op2,
 * NORM n,
 * double tol - accuracy,
 * Logger* pLogger
 *
 * output:
 * bool - return true if n norm of difference vectors op1 and op2 less then accuracy else return false
 */
bool IVector::equals(IVector const* const& op1, IVector const* const& op2, NORM n, double tol)
{
    if (op1 == nullptr || op2 == nullptr)
    {
        Vector::log(RC::NULLPTR_ERROR, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return false;
    }
    if (op1->getDim() != op2->getDim())
    {
        Vector::log(RC::MISMATCHING_DIMENSIONS, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return false;
    }
    size_t dim = op1->getDim();
    double const* dataOp1 = op1->getData();
    double const* dataOp2 = op2->getData();

    if (dataOp1 == nullptr || dataOp2 == nullptr)
    {
        Vector::log(RC::NULLPTR_ERROR, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return false;
    }
    double _dist = -1.;
    if (n == NORM::FIRST)
        _dist = Vector::distFirstNorm(dim, dataOp1, dataOp2);
    if (n == NORM::SECOND)
        _dist = Vector::distSecondNorm(dim, dataOp1, dataOp2);
    if (n == NORM::CHEBYSHEV)
        _dist = Vector::distInfinityNorm(dim, dataOp1, dataOp2);

    if (_dist < 0 || _dist > tol)
        return false;

    return true;
}


RC IVector::copyInstance(IVector* const dest, IVector const * const& src)
{
    if (src == nullptr || dest == nullptr)
    {
        Vector::log(RC::NULLPTR_ERROR, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return RC::NULLPTR_ERROR;
    }

    size_t offset = std::abs(reinterpret_cast<int64_t>(dest) - reinterpret_cast<int64_t>(src));
    if (offset < dest->sizeAllocated())
    {
        Vector::log(RC::ALLOCATION_ERROR, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return RC::ALLOCATION_ERROR;
    }

    std::memcpy(dest, src, src->sizeAllocated());
    return RC::SUCCESS;
}

RC IVector::moveInstance(IVector* const dest, IVector*& src)
{
    if (src == nullptr || dest == nullptr)
    {
        Vector::log(RC::NULLPTR_ERROR, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return RC::NULLPTR_ERROR;
    }
    std::memmove(dest, src, src->sizeAllocated());

    return RC::SUCCESS;
}

/**
 * input:
 *
 * output:
 * return copy of vector
 */
IVector* Vector::clone() const
{
    return IVector::createVector(_dim, this->getData());
}

/**
 * input:
 *
 * output:
 * return pointer of vector coordinates double array
 */
inline double* Vector::getDataPointer()
{
    return reinterpret_cast<double*>(reinterpret_cast<uint8_t*>(this) + sizeof(Vector));
}


/**
 * input:
 * size_t index - coordinate index
 *
 * output:
 * double& val - value if index coordinate
 * RC - return code, RC::SUCCESS if operation finished success
 */
RC Vector::getCord(size_t index, double& val) const
{
    if (index >= _dim)
    {
        log(RC::INDEX_OUT_OF_BOUND, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return RC::INDEX_OUT_OF_BOUND;
    }
    double const* data = this->getData();
    if (data == nullptr)
    {
        log(RC::NULLPTR_ERROR, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return RC::NULLPTR_ERROR;
    }
    val = data[index];

    return RC::SUCCESS;
}

/**
 * input:
 * size_t index - coordinate index
 * double val - the value to be assigned to the vector coordinate
 *
 * output:
 * RC - return code, RC::SUCCESS if operation finished success
 */
RC Vector::setCord(size_t index, double val)
{
    if (index >= _dim)
    {
        log(RC::INDEX_OUT_OF_BOUND, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return RC::INDEX_OUT_OF_BOUND;
    }
    if (isnan(val))
    {
        log(RC::NOT_NUMBER, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return RC::NOT_NUMBER;
    }

    double* data = this->getDataPointer();
    if (data == nullptr)
    {
        log(RC::NULLPTR_ERROR, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return RC::NULLPTR_ERROR;
    }

    data[index] = val;

    return RC::SUCCESS;
}

/**
 * input:
 * double multiplier - multiplier for all vector coordinates
 *
 * output:
 * RC - return code, RC::SUCCESS, if operation finished success
 */
RC Vector::scale(double multiplier)
{
    if (isnan(multiplier))
    {
        log(RC::NOT_NUMBER, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return RC::NOT_NUMBER;
    }

    double* data = this->getDataPointer();
    if (data == nullptr)
    {
        log(RC::NULLPTR_ERROR, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return RC::NULLPTR_ERROR;
    }

    for (size_t i = 0; i < _dim; i++)
        data[i] *= multiplier;

    return RC::SUCCESS;
}

size_t Vector::getDim() const
{
    return _dim;
}

/**
 * input:
 * NORM n
 *
 * output:
 * double - value of n vector norm, return not NAN if operation finished success
 */
double Vector::norm(NORM n) const
{
    double const* data = this->getData();
    if (data == nullptr)
    {
        log(RC::NOT_NUMBER, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return NAN;
    }

    if (n == NORM::FIRST)
        return distFirstNorm(getDim(), data);
    if (n == NORM::SECOND)
        return distSecondNorm(getDim(), data);
    if (n == NORM::CHEBYSHEV)
        return distInfinityNorm(getDim(), data);

    return NAN;
}

/**
 * input:
 * size_t dim,
 * double const* dataOp1 - first vector coordinates array,
 * double const* dataOp2 - second vector coordinates array, has default value equal nullptr,
 *
 * output:
 * if dataOp2 equal nullptr, return first norm of vecotor with dimension == dim and coordinates array dataOp1
 * if dataOp2 not equal nullptr return first norm of vector difference with coordinates arrays dataOp1 and dataOp2 and dimension equal dim
 */
double Vector::distFirstNorm(size_t dim, double const* dataOp1, double const* dataOp2)
{
    if (dataOp1 == nullptr)
    {
        log(RC::NULLPTR_ERROR, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return NAN;
    }

    double _dist = 0.;
    double center = 0.;
    for (size_t i = 0; i < dim; i++)
    {
        if (dataOp2 != nullptr)
            center = dataOp2[i];
        _dist += fabs(dataOp1[i] -  center);
    }

    return _dist;
}

/**
 * input:
 * size_t dim,
 * double const* dataOp1 - first vector coordinates array,
 * double const* dataOp2 - second vector coordinates array, has default value equal nullptr,
 *
 * output:
 * if dataOp2 equal nullptr, return second norm of vecotor with dimension == dim and coordinates array dataOp1
 * if dataOp2 not equal nullptr return second norm of vector difference with coordinates arrays dataOp1 and dataOp2 and dimension equal dim
 */
double Vector::distSecondNorm(size_t dim, double const* dataOp1, double const* dataOp2)
{
    if (dataOp1 == nullptr)
    {
        log(RC::NOT_NUMBER, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return NAN;
    }

    double _dist = 0.;
    double center = 0.;
    for (size_t i = 0; i < dim; i++)
    {
        if (dataOp2 != nullptr)
            center = dataOp2[i];
        _dist += (dataOp1[i] -  center) * (dataOp1[i] - center);
    }

    _dist = sqrt(_dist);

    return _dist;
}

/**
 * input:
 * size_t dim,
 * double const* dataOp1 - first vector coordinates array,
 * double const* dataOp2 - second vector coordinates array, has default value equal nullptr,
 *
 * output:
 * if dataOp2 equal nullptr, return infinity norm of vecotor with dimension == dim and coordinates array dataOp1
 * if dataOp2 not equal nullptr return infinity norm of vector difference with coordinates arrays dataOp1 and dataOp2 and dimension equal dim
 * if dataOp2 not equal nullptr return infinity norm of vector difference with coordinates arrays dataOp1 and dataOp2 and dimension equal dim
 */
double Vector::distInfinityNorm(size_t dim, double const* dataOp1, double const* dataOp2)
{
    if (dataOp1 == nullptr)
    {
        log(RC::NOT_NUMBER, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return NAN;
    }

    double _dist = 0.;
    double center = 0.;
    for (size_t i = 0; i < dim; i++)
    {
        if (dataOp2 != nullptr)
            center = dataOp2[i];
        if (_dist < fabs(dataOp1[i] - center))
            _dist = fabs(dataOp1[i] - center);
    }

    return _dist;
}

/**
 * input:
 * const std::function<void(double)>& fun - function which will apply to all elements of vector

 * output:
 * RC - return code
 */

RC Vector::foreach(const std::function<void(double)>& fun) const
{
    double const* data = this->getData();
    for (size_t i = 0; i < getDim(); i++)
        fun(data[i]);

    return RC::SUCCESS;
}

/**
 * input:
 * const std::function<double(double)>& fun -  function which will apply to all elements of vector
 *
 * output:
 * RC - return code
 */
RC Vector::applyFunction(const std::function<double(double)>& fun)
{
    RC rc = RC::SUCCESS;
    foreach([&rc, fun](double x){
        if (isnan(x)) rc = RC::NOT_NUMBER;
        if (isinf(x)) rc = RC::INFINITY_OVERFLOW;
    });
    if (rc != RC::SUCCESS)
    {
        log(rc, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return rc;
    }

    double* data = this->getDataPointer();
    for (size_t i = 0; i < getDim(); i++)
        data[i] = fun(data[i]);

    return RC::SUCCESS;
}

/**
 * input:
 * IVector const* const& op - vector which will add to this vector
 * double multiplier - multiplier of additional vector
 *
 * output:
 * RC - return code
 */
RC Vector::adder(IVector const* const& op, double multiplier)
{
    if (op == nullptr)
    {
        log(RC::NULLPTR_ERROR, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return RC::NULLPTR_ERROR;
    }
    if (op->getDim() != getDim())
    {
        log(RC::MISMATCHING_DIMENSIONS, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return RC::MISMATCHING_DIMENSIONS;
    }

    double const* dataOp = op->getData();
    double* data = this->getDataPointer();

    if (dataOp == nullptr || data == nullptr)
        return RC::NULLPTR_ERROR;

    for (size_t i = 0; i < getDim(); i++)
    {
        if (isnan(data[i] + multiplier * dataOp[i]))
        {
            log(RC::NOT_NUMBER, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
            return RC::NOT_NUMBER;
        }
        if (isinf(data[i] + multiplier * dataOp[i]))
        {
            log(RC::INFINITY_OVERFLOW, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
            return RC::INFINITY_OVERFLOW;
        }
    }

    for (size_t i = 0; i < getDim(); i++)
        data[i] = data[i] + multiplier * dataOp[i];

    return RC::SUCCESS;
}

RC Vector::inc(IVector const* const& op)
{
    RC rc = adder(op, 1.);
    if (rc != RC::SUCCESS)
        log(rc, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
    return rc;
}

RC Vector::dec(IVector const* const& op)
{
    RC rc = adder(op, -1.);
    if (rc != RC::SUCCESS)
        log(rc, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
    return rc;
}

double const* Vector::getData() const
{
    return reinterpret_cast<double const*>(reinterpret_cast<uint8_t const*>(this) + sizeof(Vector));
}

size_t Vector::sizeAllocated() const
{
    return getDim() * sizeof(double) + sizeof(Vector);
}

RC Vector::setLoggerImpl(ILogger* const logger)
{
    if (logger == nullptr)
        return RC::NULLPTR_ERROR;
    Vector::logger = logger;
    return RC::SUCCESS;
}

void Vector::log(RC code, ILogger::Level level, const char* const& srcfile, const char* const& function, int line)
{
    if (logger != nullptr)
        logger->log(code, level, srcfile, function, line);
}

RC Vector::setData(size_t dim, const double *const &ptr_data) {
    if (ptr_data == nullptr)
    {
        log(RC::NULLPTR_ERROR, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return RC::NULLPTR_ERROR;
    }
    if (dim != getDim())
    {
        log(RC::MISMATCHING_DIMENSIONS, ILogger::Level::INFO, __FILE__, __func__, __LINE__);
        return RC::MISMATCHING_DIMENSIONS;
    }

    double* dest = getDataPointer();
    std::memcpy(dest, ptr_data, dim * sizeof(double));

    return RC::SUCCESS;
}

ILogger *Vector::getLogger() const {
    return logger;
}

RC IVector::setLogger(ILogger* const logger)
{
    return Vector::setLoggerImpl(logger);
}

IVector::~IVector(){}


