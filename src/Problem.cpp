#include "../include/IProblem.h"
#include "../include/IDiffProblem.h"
#include "../include/IBroker.h"
#include <cmath>

#define SendInfo(Logger, Code) if (Logger != nullptr) Logger->info((Code), __FILE__, __func__, __LINE__)
#define PARAM_SPACE_DIM 2
#define ARGS_SPACE_DIM 3


inline void vectorIsValid(IVector const * const& vec, ICompact const * const& space, RC& result){
    if (result != RC::SUCCESS)
        return;
    if (vec == nullptr || space == nullptr)
        result = RC::NULLPTR_ERROR;
    else if (space->getDim() != vec->getDim())
        result = RC::MISMATCHING_DIMENSIONS;
    else if (space->isInside(vec))
        result = RC::INVALID_ARGUMENT;
}

inline void indexIsValid(IMultiIndex const* const& index, ICompact const* const& space, RC& result){
    if (result != RC::SUCCESS)
        return;
    if (index == nullptr || space == nullptr)
        result = RC::NULLPTR_ERROR;
    else if (index->getDim() != space->getDim())
        result = RC::MISMATCHING_DIMENSIONS;
}

inline void spaceIsValid(ICompact const * const& space, size_t dim, RC& result){
    if (result != RC::SUCCESS)
        return;
    if (space == nullptr)
        result = RC::NULLPTR_ERROR;
    else if (space->getDim() != dim)
        result = RC::MISMATCHING_DIMENSIONS;
}

namespace {
    class DiffProblem : public IDiffProblem{
    private:
        ICompact* _paramDomain;
        ICompact* _argsDomain;
        IVector* _param;
        IVector* _args;

    public:

        DiffProblem();

        ~DiffProblem();

        IDiffProblem* clone() const override;

        bool isValidParams(IVector const * const &params) const override;

        bool isValidArgs(IVector const * const &args) const override;

        RC setParams(IVector const * const &params) override;

        RC setArgs(IVector const * const &args) override;

        RC setParamsDomain(ICompact const * const &params) override;

        RC setArgsDomain(ICompact const * const &args, ILogger* logger) override;

        double evalByArgs(IVector const * const &args) const override;

        double evalByParams(IVector const * const &params) const override;

        double evalDerivativeByArgs(IVector const * const &args, IMultiIndex const * const &index) const override;

        double evalDerivativeByParams(IVector const * const &params, IMultiIndex const * const &index) const override;

        RC evalGradientByArgs(IVector const * const &args, IVector * const &val) const override;

        RC evalGradientByParams(IVector const * const &params, IVector * const &val) const override;

        static ILogger* _logger;
    };

    ILogger* DiffProblem::_logger = nullptr;
}

IProblem::~IProblem() = default;

IDiffProblem::~IDiffProblem() = default;

DiffProblem::DiffProblem() :
    _args(nullptr),
    _param(nullptr),
    _argsDomain(nullptr),
    _paramDomain(nullptr)
{
}

DiffProblem::~DiffProblem() {
    delete _args;
    delete _param;
    delete _argsDomain;
    delete _paramDomain;
}

IDiffProblem* IDiffProblem::createDiffProblem() {
    return new(std::nothrow) DiffProblem();
}

RC IDiffProblem::setLogger(ILogger *const logger) {
    if (logger == nullptr){
        SendInfo(DiffProblem::_logger, RC::NULLPTR_ERROR);
        return RC::NULLPTR_ERROR;
    }
    DiffProblem::_logger = logger;
    return RC::SUCCESS;
}

ILogger* IDiffProblem::getLogger() {
    return DiffProblem::_logger;
}

template <typename T>
inline T* cloneObject(T* obj, RC& rc){
    if (obj == nullptr)
        return nullptr;
    auto objClone = obj->clone();
    if (objClone == nullptr){
        rc = RC::NULLPTR_ERROR;
        return nullptr;
    }
    return objClone;
}

IDiffProblem* DiffProblem::clone() const {
    RC objClonesRC = RC::SUCCESS;
    auto args = cloneObject(_args, objClonesRC);
    auto params = cloneObject(_param, objClonesRC);
    auto argsDom = cloneObject(_argsDomain, objClonesRC);
    auto paramDom = cloneObject(_paramDomain, objClonesRC);
    auto problem = new(std::nothrow) DiffProblem();
    if (objClonesRC != RC::SUCCESS || problem == nullptr){
        SendInfo(_logger, objClonesRC);
        delete problem;
        delete args;
        delete params;
        delete argsDom;
        delete paramDom;
        return nullptr;
    }
    problem->_args = args;
    problem->_param = params;
    problem->_argsDomain = argsDom;
    problem->_paramDomain = paramDom;
    return problem;
}

bool DiffProblem::isValidParams(const IVector *const &params) const {
    RC validCallRC = RC::SUCCESS;
    vectorIsValid(params, _paramDomain, validCallRC);
    if (validCallRC != RC::SUCCESS){
        SendInfo(_logger, validCallRC);
        return false;
    }
    return true;
}

bool DiffProblem::isValidArgs(const IVector *const &args) const {
    RC validCallRC = RC::SUCCESS;
    vectorIsValid(args, _argsDomain, validCallRC);
    if (validCallRC != RC::SUCCESS){
        SendInfo(_logger, validCallRC);
        return false;
    }
    return true;
}

inline RC setVector(IVector const* const& newVec, IVector*& vec, ICompact const* const& space){
    RC validCallRC = RC::SUCCESS;
    vectorIsValid(newVec, space, validCallRC);
    if (validCallRC != RC::SUCCESS)
        return validCallRC;
    auto newVecClone = newVec->clone();
    if (newVecClone == nullptr)
        return RC::NULLPTR_ERROR;
    delete vec;
    vec = newVecClone;
    return RC::SUCCESS;
}

RC DiffProblem::setParams(const IVector *const &params) {
    RC setVectorRC = setVector(params, _param, _paramDomain);
    if (setVectorRC != RC::SUCCESS)
        SendInfo(_logger, setVectorRC);
    return setVectorRC;
}

RC DiffProblem::setArgs(const IVector *const &args) {
    RC setVectorRC = setVector(args, _args, _argsDomain);
    if (setVectorRC != RC::SUCCESS)
        SendInfo(_logger, setVectorRC);
    return setVectorRC;
}

inline RC setDomain(ICompact const* const& newSpace, ICompact*& space, size_t spaceDim){
    RC validCallRC = RC::SUCCESS;
    spaceIsValid(newSpace, spaceDim, validCallRC);
    if (validCallRC != RC::SUCCESS)
        return validCallRC;
    auto newSpaceClone = newSpace->clone();
    if (newSpaceClone == nullptr)
        return RC::NULLPTR_ERROR;
    delete space;
    space = newSpaceClone;
    return RC::SUCCESS;
}

RC DiffProblem::setParamsDomain(const ICompact *const &params) {
    RC setDomainRC = setDomain(params, _paramDomain, PARAM_SPACE_DIM);
    if (setDomainRC != RC::SUCCESS){
        SendInfo(_logger, setDomainRC);
        return setDomainRC;
    }
    delete _param;
    _param = nullptr;
    return RC::SUCCESS;
}

RC DiffProblem::setArgsDomain(const ICompact *const &args, ILogger *logger) {
    RC setArgsRC = setDomain(args, _argsDomain, ARGS_SPACE_DIM);
    if (setArgsRC != RC::SUCCESS) {
        SendInfo(logger, setArgsRC);
        return setArgsRC;
    }
    delete _args;
    _args = nullptr;
    return setArgsRC;
}

inline double stretchedCos(double x, double p, size_t dx = 0){
    return std::pow(p, static_cast<double>(dx)) * std::cos(x * p + static_cast<double>(dx % 4) * M_PI / 2.);
}

inline double myFunction(double x1, double x2, double p1, double p2, size_t dx1 = 0, size_t dx2 = 0){
    return stretchedCos(x1, p1, dx1) + stretchedCos(x2, p2, dx2);
}

inline double eval(IVector const* const& args, IVector const* const& param,
                   ICompact const* const& argsDomain){
    RC validCallRC = RC::SUCCESS;
    vectorIsValid(args, argsDomain, validCallRC);
    if (validCallRC != RC::SUCCESS)
        return NAN;
    auto _args = args->getData();
    auto _param = param->getData();
    if (_args == nullptr || _param == nullptr)
        return NAN;

    return myFunction(_args[0], _args[1], _param[0], _param[1], 0, 0);
}

double DiffProblem::evalByArgs(const IVector *const &args) const {
    if (_param == nullptr){
        SendInfo(_logger, RC::NO_PARAMS_SET);
        return NAN;
    }
    return eval(args, _param, _argsDomain);
}

double DiffProblem::evalByParams(const IVector *const &params) const {
    if (_args == nullptr){
        SendInfo(_logger, RC::NO_ARGS_SET);
        return NAN;
    }
    return eval(params, _args, _paramDomain);
}

inline double evalDerivative(IVector const* const& args, IVector const* const& params,
                             ICompact const* const& argsDomain, IMultiIndex const* const& dx){
    RC validCallRC = RC::SUCCESS;
    vectorIsValid(args, argsDomain, validCallRC);
    indexIsValid(dx, argsDomain, validCallRC);
    if (validCallRC != RC::SUCCESS){
        SendInfo(DiffProblem::_logger, validCallRC);
        return NAN;
    }
    auto _args = args->getData();
    auto _param = params->getData();
    auto _dx = dx->getData();
    if (_args == nullptr || _param == nullptr || _dx == nullptr){
        SendInfo(DiffProblem::_logger, RC::NULLPTR_ERROR);
        return NAN;
    }
    return myFunction(_args[0], _args[1], _param[0], _param[1], _dx[0], _dx[1]);
}

double DiffProblem::evalDerivativeByArgs(const IVector *const &args, const IMultiIndex *const &index) const {
    if (_param == nullptr){
        SendInfo(_logger, RC::NO_PARAMS_SET);
        return NAN;
    }
    return evalDerivative(args, _param, _paramDomain, index);
}

double DiffProblem::evalDerivativeByParams(const IVector *const &params, const IMultiIndex *const &index) const {
    if (_args == nullptr){
        SendInfo(_logger, RC::NO_ARGS_SET);
        return NAN;
    }
    return evalDerivative(params, _args, _argsDomain, index);
}

inline RC evalGradient(IVector const* const& args, IVector const* const& params, IVector *const& val){
    if (args == nullptr || params == nullptr || val == nullptr){
        SendInfo(DiffProblem::_logger, RC::NULLPTR_ERROR);
        return RC::NULLPTR_ERROR;
    }
    auto argsData = args->getData();
    auto paramsData = params->getData();
    double derivValue1 = myFunction(argsData[0], argsData[1], paramsData[0], paramsData[1], 1, 0);
    double derivValue2 = myFunction(argsData[0], argsData[1], paramsData[0], paramsData[1], 0, 1);
    if (std::isinf(derivValue1) || std::isnan(derivValue1) ||
        std::isinf(derivValue2) || std::isnan(derivValue2)){
        SendInfo(DiffProblem::_logger, RC::NOT_NUMBER);
        return RC::NOT_NUMBER;
    }
    RC setValueRC;
    setValueRC = val->setCord(0, derivValue1);
    if (setValueRC != RC::SUCCESS){
        SendInfo(DiffProblem::_logger, setValueRC);
        return setValueRC;
    }
    setValueRC = val->setCord(2, derivValue2);
    if (setValueRC != RC::SUCCESS){
        SendInfo(DiffProblem::_logger, setValueRC);
        return setValueRC;
    }
    return RC::SUCCESS;
}

RC DiffProblem::evalGradientByArgs(const IVector *const &args, IVector *const &val) const {
    RC validCallRC = RC::SUCCESS;
    vectorIsValid(args, _argsDomain, validCallRC);
    if (val == nullptr)
        validCallRC = RC::NULLPTR_ERROR;
    if (validCallRC == RC::SUCCESS && val->getDim() != args->getDim())
        validCallRC = RC::MISMATCHING_DIMENSIONS;
    if (validCallRC != RC::SUCCESS){
        SendInfo(_logger, validCallRC);
        return validCallRC;
    }

    return evalGradient(args, _param, val);
}

RC DiffProblem::evalGradientByParams(const IVector *const &params, IVector *const &val) const {
    RC validCallRC = RC::SUCCESS;
    vectorIsValid(params, _paramDomain, validCallRC);
    if (val == nullptr)
        validCallRC = RC::NULLPTR_ERROR;
    if (validCallRC == RC::SUCCESS && val->getDim() != params->getDim())
        validCallRC = RC::MISMATCHING_DIMENSIONS;
    if (validCallRC != RC::SUCCESS){
        SendInfo(_logger, validCallRC);
        return validCallRC;
    }

    return evalGradient(params, _args, val);
}


namespace {
    class Broker : public IBroker{
    private:
        static Broker* _instance;
        static const IBroker::INTERFACE_IMPL _interfaceImpl = IBroker::INTERFACE_IMPL::IPROBLEM;
    public:

        static Broker* instance();

        bool canCastTo(INTERFACE_IMPL impl) const override;

        void* getInterfaceImpl(INTERFACE_IMPL impl) const override;

        void release() override;

        ~Broker() override;
    };

    Broker* Broker::_instance = nullptr;
}

extern "C" {
    LIB_EXPORT void* getBroker() {
        return (void*) Broker::instance();
    }
}

IBroker::~IBroker() = default;

Broker::~Broker() = default;

Broker *Broker::instance() {
    if (_instance == nullptr)
        _instance = new (std::nothrow) Broker();
    return _instance;
}

bool Broker::canCastTo(IBroker::INTERFACE_IMPL impl) const {
    return impl == _interfaceImpl;
}

void* Broker::getInterfaceImpl(IBroker::INTERFACE_IMPL impl) const {
    return impl == _interfaceImpl ? IDiffProblem::createDiffProblem() : nullptr;
}

void Broker::release() {
    delete _instance;
    _instance = nullptr;
}

/*
 *
 */

namespace {
    class Problem : public IProblem{
    private:
        ICompact* _paramDomain;
        ICompact* _argsDomain;
        IVector* _param;
        IVector* _args;

    public:

        Problem();

        ~Problem();

        IProblem* clone() const override;

        bool isValidParams(IVector const * const &params) const override;

        bool isValidArgs(IVector const * const &args) const override;

        RC setParams(IVector const * const &params) override;

        RC setArgs(IVector const * const &args) override;

        RC setParamsDomain(ICompact const * const &params) override;

        RC setArgsDomain(ICompact const * const &args, ILogger* logger) override;

        double evalByArgs(IVector const * const &args) const override;

        double evalByParams(IVector const * const &params) const override;

        static ILogger* _logger;
    };

    ILogger* Problem::_logger = nullptr;
}

Problem::Problem() :
        _args(nullptr),
        _param(nullptr),
        _argsDomain(nullptr),
        _paramDomain(nullptr)
{
}

Problem::~Problem() {
    delete _args;
    delete _param;
    delete _argsDomain;
    delete _paramDomain;
}

IProblem* IProblem::createProblem() {
    return new(std::nothrow) Problem();
}

RC IProblem::setLogger(ILogger *const logger) {
    if (logger == nullptr){
        SendInfo(Problem::_logger, RC::NULLPTR_ERROR);
        return RC::NULLPTR_ERROR;
    }
    Problem::_logger = logger;
    return RC::SUCCESS;
}

ILogger* IProblem::getLogger() {
    return Problem::_logger;
}

IProblem* Problem::clone() const {
    RC objClonesRC = RC::SUCCESS;
    auto args = cloneObject(_args, objClonesRC);
    auto params = cloneObject(_param, objClonesRC);
    auto argsDom = cloneObject(_argsDomain, objClonesRC);
    auto paramDom = cloneObject(_paramDomain, objClonesRC);
    auto problem = new(std::nothrow) Problem();
    if (objClonesRC != RC::SUCCESS || problem == nullptr){
        SendInfo(_logger, objClonesRC);
        delete problem;
        delete args;
        delete params;
        delete argsDom;
        delete paramDom;
        return nullptr;
    }
    problem->_args = args;
    problem->_param = params;
    problem->_argsDomain = argsDom;
    problem->_paramDomain = paramDom;
    return problem;
}

bool Problem::isValidParams(const IVector *const &params) const {
    RC validCallRC = RC::SUCCESS;
    vectorIsValid(params, _paramDomain, validCallRC);
    if (validCallRC != RC::SUCCESS){
        SendInfo(_logger, validCallRC);
        return false;
    }
    return true;
}

bool Problem::isValidArgs(const IVector *const &args) const {
    RC validCallRC = RC::SUCCESS;
    vectorIsValid(args, _argsDomain, validCallRC);
    if (validCallRC != RC::SUCCESS){
        SendInfo(_logger, validCallRC);
        return false;
    }
    return true;
}

RC Problem::setParams(const IVector *const &params) {
    RC setVectorRC = setVector(params, _param, _paramDomain);
    if (setVectorRC != RC::SUCCESS)
        SendInfo(_logger, setVectorRC);
    return setVectorRC;
}

RC Problem::setArgs(const IVector *const &args) {
    RC setVectorRC = setVector(args, _args, _argsDomain);
    if (setVectorRC != RC::SUCCESS)
        SendInfo(_logger, setVectorRC);
    return setVectorRC;
}

RC Problem::setParamsDomain(const ICompact *const &params) {
    RC setDomainRC = setDomain(params, _paramDomain, PARAM_SPACE_DIM);
    if (setDomainRC != RC::SUCCESS){
        SendInfo(_logger, setDomainRC);
        return setDomainRC;
    }
    delete _param;
    _param = nullptr;
    return RC::SUCCESS;
}

RC Problem::setArgsDomain(const ICompact *const &args, ILogger *logger) {
    RC setArgsRC = setDomain(args, _argsDomain, ARGS_SPACE_DIM);
    if (setArgsRC != RC::SUCCESS) {
        SendInfo(logger, setArgsRC);
        return setArgsRC;
    }
    delete _args;
    _args = nullptr;
    return setArgsRC;
}

double Problem::evalByArgs(const IVector *const &args) const {
    if (_param == nullptr){
        SendInfo(_logger, RC::NO_PARAMS_SET);
        return NAN;
    }
    return eval(args, _param, _argsDomain);
}

double Problem::evalByParams(const IVector *const &params) const {
    if (_args == nullptr){
        SendInfo(_logger, RC::NO_ARGS_SET);
        return NAN;
    }
    return eval(params, _args, _paramDomain);
}
