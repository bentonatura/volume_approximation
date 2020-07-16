// VolEsti (volume computation and sampling library)
//
// Copyright (c) 2020 Bento Natura
//
// Licensed under GNU LGPL.3, see LICENCE file

#include<iostream>
#include "LHSCB.h"

Vector all_ones_vector(const int n) {
    Vector v(n);
    for (int i = 0; i < n; ++i) {
        v[i] = 1.0;
    }
    return v;
}

Matrix LHSCB::inverse_hessian(Vector x) {
    Eigen::LLT<Matrix> LLT = llt(x);
    Matrix L_inv = LLT.matrixL().toDenseMatrix().inverse();
    return L_inv.transpose() * L_inv;
}

unsigned int LHSCB::getNumVariables() const {
    return _num_variables;
}

//TODO: use short queue instead.
Vector *LHSCB::find_gradient(Vector x) {
    for (int i = _stored_gradients.size() - 1; i >= 0; i--) {
        if (x == _stored_gradients[i].first) {
            return &_stored_gradients[i].second;
        }
    }
    return nullptr;
}

Matrix *LHSCB::find_hessian(Vector x) {
    for (int i = _stored_hessians.size() - 1; i >= 0; i--) {
        if (x == _stored_hessians[i].first) {
            return &_stored_hessians[i].second;
        }
    }
    return nullptr;
}

Eigen::LLT<Matrix> *LHSCB::find_LLT(Vector x) {
    for (int i = _stored_LLT.size() - 1; i >= 0; i--) {
        if (x == _stored_LLT[i].first) {
            return &_stored_LLT[i].second;
        }
    }
    return nullptr;
}

Eigen::LLT<Matrix> ProductBarrier::llt(Vector x, bool symmetrize) {
    //TODO: Figure out how to write Eigen::LLT<Matrix> in Matrix form.
    return LHSCB::llt(x, symmetrize);
}

//TODO: code resembles code for gradient and other methods. Find abstraction.
Vector ProductBarrier::llt_L_solve(Vector x, Vector rhs) {
    unsigned idx = 0;
    Vector product_llt_solve(_num_variables);
    for (unsigned i = 0; i < _barriers.size(); ++i) {
        LHSCB *barrier = _barriers[i];
        unsigned num_variables = _num_vars_per_barrier[i];
        Vector x_seg = x.segment(idx, num_variables);
        Vector rhs_seg = rhs.segment(idx, num_variables);
        Vector lls_solve_seg = barrier->llt_L_solve(x_seg, rhs_seg);
        product_llt_solve.segment(idx, num_variables) = lls_solve_seg;
        idx += num_variables;
    }
    return product_llt_solve;
}

//TODO: code resembles code for gradient and other methods. Find abstraction.
Matrix ProductBarrier::llt_solve(Vector x, const Matrix &rhs) {
    unsigned idx = 0;
    Matrix product_llt_solve = Matrix::Zero(rhs.rows(), rhs.cols());
    for (unsigned i = 0; i < _barriers.size(); ++i) {
        LHSCB *barrier = _barriers[i];
        unsigned num_variables = _num_vars_per_barrier[i];
        Vector x_seg = x.segment(idx, num_variables);
        Matrix rhs_block = rhs.block(idx, 0, num_variables, rhs.cols());
        Matrix lls_solve_block = barrier->llt_solve(x_seg, rhs_block);
        product_llt_solve.block(idx, 0, num_variables, rhs.cols()) = lls_solve_block;
        idx += num_variables;
    }
    return product_llt_solve;
}

Eigen::LLT<Matrix> LHSCB::llt(Vector x, bool symmetrize) {

    Eigen::LLT<Matrix> *llt_ptr = nullptr;

    if (not symmetrize) {
        llt_ptr = find_LLT(x);
    }

    if (llt_ptr) {
        return *llt_ptr;
    }

    Eigen::LLT<Matrix> *llt_var;

    if (not symmetrize) {
        llt_var = new Eigen::LLT<Matrix>(hessian(x).llt());
    } else {
        llt_var = new Eigen::LLT<Matrix>(((hessian(x) + hessian(x).transpose()) / 2).llt());
    }

    if (_stored_LLT.empty()) {
        _stored_LLT.resize(1);
    }
    _stored_LLT[0] = std::pair<Vector, Eigen::LLT<Matrix> >(x, *llt_var);
    return *llt_var;
}

Matrix LHSCB::llt_solve(Vector x, const Matrix &rhs) {
    return llt(x).solve(rhs);
}

Vector LHSCB::llt_L_solve(Vector x, Vector rhs) {
    return llt(x).matrixL().solve(rhs);
}

//LP Standard Log Barrier

bool LPStandardBarrier::in_interior(Vector x) {
    return (x.minCoeff() > 0);
}

Vector LPStandardBarrier::gradient(Vector x) {
    assert(in_interior(x));
    return -x.array().inverse();
}

Matrix LPStandardBarrier::hessian(Vector x) {
    return x.array().pow(2).inverse().matrix().asDiagonal();
}

Matrix LPStandardBarrier::inverse_hessian(Vector x) {

    const Matrix &inverse_hessian = x.array().pow(2).matrix().asDiagonal();
//    std::cout << "Inverse hessian: " << std::endl << inverse_hessian << std::endl;
    return inverse_hessian;
}

IPMDouble LPStandardBarrier::concordance_parameter(Vector x) {
    return x.rows();
}

Vector LPStandardBarrier::initialize_x() {
    return all_ones_vector(_num_variables);
}

Vector LPStandardBarrier::initialize_s() {
    return all_ones_vector(_num_variables);
}

//SDP Standard Log Barrier

//TODO: check whether implementation works correctly

bool SDPStandardBarrier::in_interior(Vector x) {
    Matrix X = toMatrix(x);
    auto LLT = X.llt();
    if (LLT.info() != Eigen::NumericalIssue) {
        return true;
    }
//    std::cout << "determinant of" << std::endl << X << std::endl << "is not positive, but "
//    << X.determinant() << std::endl;
    return false;
}

Vector SDPStandardBarrier::gradient(Vector x) {
    assert(in_interior(x));
    Matrix X = toMatrix(x);
    Matrix X_Inv = X.inverse();
    Vector x_inv = toVector(X_Inv);
    return -x_inv;
}

Matrix SDPStandardBarrier::hessian(Vector x) {
    assert(in_interior(x));
    Matrix X = toMatrix(x);
    Matrix X_Inv = X.inverse();

    Matrix H(_matrix_dimension * _matrix_dimension, _matrix_dimension * _matrix_dimension);
    for (unsigned i = 0; i < _matrix_dimension; i++) {
        for (unsigned j = 0; j < _matrix_dimension; ++j) {
            Matrix x_ij = X_Inv.col(j) * X_Inv.row(i);
            Eigen::Map<Matrix> x_ij_row(x_ij.data(), _matrix_dimension * _matrix_dimension, 1);
            H.col(i * _matrix_dimension + j) = x_ij_row;
        }
    }
    return H;
}

//TODO: figure out what correct concordance for SDP is.
IPMDouble SDPStandardBarrier::concordance_parameter(Vector) {
    return _matrix_dimension;
}

Vector SDPStandardBarrier::toVector(Matrix X) {
    assert(X.rows() == _matrix_dimension and X.cols() == _matrix_dimension);
    return MatrixToVector(X);
}

Matrix SDPStandardBarrier::toMatrix(Vector x) {
    assert(x.rows() == _matrix_dimension * _matrix_dimension);
    return VectorToSquareMatrix(x, _matrix_dimension);
}

Vector SDPStandardBarrier::initialize_x() {
    Matrix Id = Matrix::Identity(_matrix_dimension, _matrix_dimension);
    Eigen::Map<Matrix> Id_stacked(Id.data(), _matrix_dimension * _matrix_dimension, 1);
    return Id_stacked;
}

Vector SDPStandardBarrier::initialize_s() {
    return initialize_x();
}

//TODO: Nicer implementation of all the product operations.

Vector ProductBarrier::gradient(Vector x) {
    unsigned idx = 0;
    Vector product_gradient(_num_variables);
    for (unsigned i = 0; i < _barriers.size(); ++i) {
        LHSCB *barrier = _barriers[i];
        unsigned num_variables = _num_vars_per_barrier[i];
        Vector v_segment = x.segment(idx, num_variables);
        Vector gradient_segment = barrier->gradient(v_segment);
        product_gradient.segment(idx, num_variables) = gradient_segment;
        idx += num_variables;
    }
    return product_gradient;
}

Matrix ProductBarrier::hessian(Vector x) {
    unsigned idx = 0;
    Matrix product_hessian = Matrix::Zero(_num_variables, _num_variables);
    for (unsigned i = 0; i < _barriers.size(); ++i) {
        LHSCB *barrier = _barriers[i];
        unsigned num_variables = _num_vars_per_barrier[i];
        Vector v_segment = x.segment(idx, num_variables);
        Matrix hessian_block = barrier->hessian(v_segment);
        product_hessian.block(idx, idx, num_variables, num_variables) = hessian_block;
        idx += num_variables;
    }
    return product_hessian;
}

bool ProductBarrier::in_interior(Vector x) {
    _in_interior_timer.start();
    unsigned idx = 0;
    for (unsigned i = 0; i < _barriers.size(); ++i) {
        LHSCB *barrier = _barriers[i];
        unsigned num_variables = _num_vars_per_barrier[i];
        Vector v_segment = x.segment(idx, num_variables);
        if (not barrier->in_interior(v_segment)) {
            _in_interior_timer.stop();
            return false;
        }
        idx += num_variables;
    }
    _in_interior_timer.stop();
    return true;
}

IPMDouble ProductBarrier::concordance_parameter(Vector x) {
    unsigned idx = 0;
    IPMDouble concordance_par = 0;
    for (unsigned i = 0; i < _barriers.size(); ++i) {
        LHSCB *barrier = _barriers[i];
        unsigned num_variables = _num_vars_per_barrier[i];
        Vector v_segment = x.segment(idx, num_variables);
        concordance_par += barrier->concordance_parameter(v_segment);
        idx += num_variables;
    }
    return concordance_par;
}

Vector ProductBarrier::initialize_x() {
    unsigned idx = 0;
    Vector product_init = Vector(_num_variables);
    for (unsigned i = 0; i < _barriers.size(); ++i) {
        LHSCB *barrier = _barriers[i];
        unsigned num_variables = _num_vars_per_barrier[i];
        Vector init_segment = barrier->initialize_x();
        product_init.segment(idx, num_variables) = init_segment;
        idx += num_variables;
    }
    return product_init;
}

Vector ProductBarrier::initialize_s() {
    unsigned idx = 0;
    Vector product_init = Vector(_num_variables);
    for (unsigned i = 0; i < _barriers.size(); ++i) {
        LHSCB *barrier = _barriers[i];
        unsigned num_variables = _num_vars_per_barrier[i];
        Vector init_segment = barrier->initialize_s();
        product_init.segment(idx, num_variables) = init_segment;
        idx += num_variables;
    }
    return product_init;
}

Matrix ProductBarrier::inverse_hessian(Vector x) {
    unsigned idx = 0;
    Matrix product_inverse_hessian = Matrix::Zero(_num_variables, _num_variables);
    for (unsigned i = 0; i < _barriers.size(); ++i) {
        LHSCB *barrier = _barriers[i];
        unsigned num_variables = _num_vars_per_barrier[i];
        Vector v_segment = x.segment(idx, num_variables);
        Matrix inverse_hessian_block = barrier->inverse_hessian(v_segment);
        product_inverse_hessian.block(idx, idx, num_variables, num_variables) = inverse_hessian_block;
        idx += num_variables;
    }
    return product_inverse_hessian;
}

//Should not be used. Reformulate instance instead.

Vector FullSpaceBarrier::gradient(Vector) {
    return Vector::Zero(_num_variables);
}

Matrix FullSpaceBarrier::hessian(Vector) {
    return Matrix::Zero(_num_variables, _num_variables);
}

Matrix FullSpaceBarrier::inverse_hessian(Vector) {
    return Matrix::Zero(_num_variables, _num_variables);
}

bool FullSpaceBarrier::in_interior(Vector) {
    return true;
}

IPMDouble FullSpaceBarrier::concordance_parameter(Vector) {
    return 0;
}

Vector FullSpaceBarrier::initialize_x() {
    return Vector::Zero(_num_variables);
}

Vector FullSpaceBarrier::initialize_s() {
    return Vector::Zero(_num_variables);
}

Vector ZeroSpaceBarrier::gradient(Vector x) {
    //should not be used
    assert(false);
    return -std::numeric_limits<IPMDouble>::infinity() * Vector::Ones(x.cols());
}

Matrix ZeroSpaceBarrier::hessian(Vector x) {
    //should not be used
    assert(false);
    return std::numeric_limits<IPMDouble>::infinity() * Matrix::Identity(x.cols(), x.cols());
}

Matrix ZeroSpaceBarrier::inverse_hessian(Vector) {
    return Matrix::Zero(_num_variables, _num_variables);
}

bool ZeroSpaceBarrier::in_interior(Vector) {
    return true;
}

IPMDouble ZeroSpaceBarrier::concordance_parameter(Vector) {
    return 0;
}

Vector ZeroSpaceBarrier::initialize_x() {
    return Vector::Zero(_num_variables);
}

Vector ZeroSpaceBarrier::initialize_s() {
    return Vector::Zero(_num_variables);
}


//////////////////////////////////////////////////////////////////////////

Vector DualSOSConeBarrier::gradient(Vector x) {
    assert(in_interior(x));
    Matrix X = Lambda(x);
    Matrix Z = X.inverse();
    Vector g(x.rows());
    for (int i = 0; i < g.rows(); ++i) {
        Matrix E_i = Matrix::Zero(X.rows(), X.cols());
        for (int j = 0; j <= i; ++j) {
            E_i(j, i - j) = 1;
        }
        g(i) = -Z.cwiseProduct(E_i).sum();
    }
    return g;
}

Matrix DualSOSConeBarrier::hessian(Vector x) {
    assert(in_interior(x));
    Matrix X = Lambda(x);
    Matrix Z = X.inverse();
    Matrix H = Matrix::Zero(Z.rows(), Z.cols());
    for (int u = 0; u < H.rows(); ++u) {
        for (int v = 0; v < H.cols(); ++v) {
            IPMDouble H_uv = 0;
            for (int a = 0; a <= u; ++a) {
                for (int k = 0; k <= v; ++k) {
                    H_uv += Z(a, u - a) + Z(k, v - k);
                }
            }
        }
    }
    return H;
}

//FIXME: Wrong implementation in in_interior method.
bool DualSOSConeBarrier::in_interior(Vector x) {
    Matrix X = Lambda(x);
    return X.determinant() > 0;
}

IPMDouble DualSOSConeBarrier::concordance_parameter(Vector x) {
    return x.rows();
}

Vector DualSOSConeBarrier::initialize_x() {
    //TODO: find centered initialization
    return Vector();
}

Vector DualSOSConeBarrier::initialize_s() {
    //TODO: find centered initialization
    return Vector();
}

Matrix DualSOSConeBarrier::Lambda(Vector x) {
    assert(x.rows() == _max_polynomial_degree + 1);
    Matrix M(_max_polynomial_degree + 1, _max_polynomial_degree + 1);
    for (unsigned i = 0; i < _max_polynomial_degree + 1; ++i) {
        for (unsigned j = 0; j < _max_polynomial_degree + 1; ++j) {
            M(i, j) = x(i + j);
        }
    }
    return M;
}

//TODO: if intermediate_LLT was calculated already in a check_interior_only call, restore it for the next call.

bool InterpolantDualSOSBarrier::update_gradient_hessian_LLT(Vector x, bool check_interior_only) {
    Matrix intermediate_matrix = Matrix(_P.cols(),_P.cols());
    intermediate_matrix.triangularView<Eigen::Lower>() = _P.transpose() * x.asDiagonal() * _P;
    Eigen::LLT<Matrix> intermediate_LLT = intermediate_matrix.selfadjointView<Eigen::Lower>().llt();
    if(intermediate_LLT.info() == Eigen::NumericalIssue){
        return false;
    }

    if(check_interior_only){
        return true;
    }

    Matrix V = intermediate_LLT.matrixL().solve(_P.transpose());
    Matrix Q = Matrix(V.cols(), V.cols());
    Q.triangularView<Eigen::Lower>() = V.transpose() * V;
    Q = Q.selfadjointView<Eigen::Lower>();

    Vector gradient = -Q.diagonal();
    //TODO: store hessian as self-adjoint
    Matrix hessian = Q.cwiseProduct(Q);
    Eigen::LLT<Matrix> llt = hessian.selfadjointView<Eigen::Lower>().llt();

    if (_stored_hessians.empty()) {
        _stored_hessians.resize(1);
    }
    _stored_hessians[0] = std::pair<Vector, Matrix>(x, hessian);

    if (_stored_gradients.empty()) {
        _stored_gradients.resize(1);
    }
    _stored_gradients[0] = std::pair<Vector, Vector>(x, gradient);

    if (_stored_LLT.empty()) {
        _stored_LLT.resize(1);
    }
    _stored_LLT[0] = std::pair<Vector, Eigen::LLT<Matrix>>(x, llt);
    return true;
}

Vector InterpolantDualSOSBarrier::gradient(Vector x) {
    auto *grad_ptr = find_gradient(x);
    if (grad_ptr) {
        return *grad_ptr;
    }
    update_gradient_hessian_LLT(x);
    return _stored_gradients[0].second;
}


Matrix InterpolantDualSOSBarrier::hessian(Vector x) {
    auto *hess_ptr = find_hessian(x);
    if (hess_ptr) {
        return *hess_ptr;
    }
    update_gradient_hessian_LLT(x);
    return _stored_hessians[0].second;
}

Eigen::LLT<Matrix> InterpolantDualSOSBarrier::llt(Vector x, bool) {
    auto *llt_ptr = find_LLT(x);
    if (llt_ptr) {
        return *llt_ptr;
    }
    update_gradient_hessian_LLT(x);
    return _stored_LLT[0].second;
}

Matrix InterpolantDualSOSBarrier::inverse_hessian(Vector x) {
    //Not sure if correct method.
    Matrix L_inv = llt(x).matrixL().toDenseMatrix().inverse();
    Matrix inv = L_inv.transpose() * L_inv;
    //TODO: delete for performance
//    if((inv * hessian(x) - Matrix::Identity(x.rows(), x.rows())).norm() > 1e-2)
//    {
//        std::cout << "WARNING: error in inversion is " << (inv * hessian(x) - Matrix::Identity(x.rows(), x.rows())).norm()
//        << std::endl;
//        std::cout << "Hessian: \n" << hessian(x) << std::endl;
//        std::cout << "LLT Hessian: \n" << hessian(x).llt().matrixLLT() << std::endl;
//        std::cout << "LLT Hessian L: \n" << hessian(x).llt().matrixL().toDenseMatrix() << std::endl;
//        std::cout << "LLT Hessian U: \n" << hessian(x).llt().matrixU().toDenseMatrix() << std::endl;
//
//    }
    return inv;
}


//REMARK: Note that in_interior can return true, while llt or hessian could fail. This is due to numerical instabilities in these more complicated operations.
//Currently fixed by disabling line search.
//TODO: calculate gradient and hessian as we are alaready half way there and need to calculate the gradient for centrality anyway.
bool InterpolantDualSOSBarrier::in_interior(Vector x) {
    return update_gradient_hessian_LLT(x, true);
}

IPMDouble InterpolantDualSOSBarrier::concordance_parameter(Vector) {
    return _L;
}

Vector InterpolantDualSOSBarrier::initialize_x() {
    return Vector::Ones(_U);
}

//TODO: Fix dual initialization
Vector InterpolantDualSOSBarrier::initialize_s() {
    return -gradient(initialize_x());
}

