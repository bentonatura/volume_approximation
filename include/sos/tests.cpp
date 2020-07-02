#include "utils.cpp"
#include "NonSymmetricIPM.h"

void test_speed_of_types(int max_degree) {//Test matrix inversion runtime

    Eigen::MatrixXd double_matrix = Eigen::MatrixXd::Random(max_degree, max_degree);
    cxxtimer::Timer t1;
    t1.start();
    Eigen::MatrixXd inv_double = double_matrix.inverse();
    inv_double = inv_double.cwiseAbs();
    t1.stop();
//    Matrix Double_matrix = Matrix::Random(max_degree, max_degree);
    cxxtimer::Timer t2;
    t2.start();
    Eigen::Matrix<double, Eigen::Dynamic, 1> sol = double_matrix.householderQr().solve
            (Eigen::Matrix<double, Eigen::Dynamic, 1>::Random(double_matrix.rows()));
    t2.stop();

//    Eigen::Matrix<long double, Eigen::Dynamic, Eigen::Dynamic> long_double_matrix =
//            Eigen::Matrix<long double, Eigen::Dynamic, Eigen::Dynamic> ::Random(max_degree, max_degree);

    cxxtimer::Timer t3;
    t3.start();
//    Eigen::Matrix<long double, Eigen::Dynamic, Eigen::Dynamic>  inv_long_double = long_double_matrix.inverse();
//    inv_long_double = inv_long_double.cwiseAbs();
    t3.stop();

    std::cout << "double: " << std::numeric_limits<double>::max();
    std::cout << "long double: " << std::numeric_limits<long double>::max();

    std::cout << "Normal doubles took " << t1.count<std::chrono::milliseconds>() <<
              " \nand high prec took " << t2.count<std::chrono::milliseconds>() <<
              " \nand long double took " << t3.count<std::chrono::milliseconds>() <<
              "\n for dim " << max_degree << std::endl;
}


Constraints convert_LP_to_SDP(Matrix &A, Vector &b, Vector &c) {
    const int m = A.rows();
    const int n = A.cols();

    Constraints SDP_constraints;
    SDP_constraints.A = Matrix::Zero(m + n * n - n, n * n);

    std::cout << A << std::endl;
    for (int i = 0; i < m; ++i) {
        Matrix diag_row = A.row(i).asDiagonal();
        Eigen::Map<Matrix> diag_row_stacked(diag_row.data(), 1, n * n);
        SDP_constraints.A.block(i, 0, 1, n * n) = diag_row_stacked;
    }

    unsigned row_idx = m;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (i != j) {
                SDP_constraints.A(row_idx, n * i + j) = 1;
                row_idx++;
            }
        }
    }

    SDP_constraints.b = Vector::Zero(SDP_constraints.A.rows());
    SDP_constraints.b.block(0, 0, m, 1) = b;

    SDP_constraints.c = Vector::Zero(SDP_constraints.A.cols());
    for (int j = 0; j < n; ++j) {
        SDP_constraints.c(j + j * n) = c(j);
    }

    return SDP_constraints;
}

bool test_sdp_solver_random_lp_formulation(const int m, const int n) {
    Matrix A = Matrix::Random(m, n);
    Vector x0 = Vector::Random(n);
    Vector c = Vector::Random(n);
    //for loop to create a feasible and bounded instance
    for (int i = 0; i < n; i++) {
        x0(i) += 1;
        c(i) += 1;
    }
    Vector b = A * x0;

    Constraints constraints = convert_LP_to_SDP(A, b, c);

    SDPStandardBarrier sdp_barrier(n);

    NonSymmetricIPM ipm_sdp_solver(constraints.A, constraints.b, constraints.c, &sdp_barrier);
    ipm_sdp_solver.run_solver();
    auto sdp_solution = ipm_sdp_solver.get_solution();
    return ipm_sdp_solver.verify_solution();
}

bool test_lp_solver_random(const int m, const int n) {
    Matrix A = Matrix::Random(m, n);
    Vector x0 = Vector::Random(n);
    Vector c = Vector::Random(n);
    //for loop to create a feasible and bounded instance
    for (int i = 0; i < n; i++) {
        x0(i) += 1;
        c(i) += 1;
    }
    Vector b = A * x0;

    LPStandardBarrier lp_barrier(n);

    NonSymmetricIPM lp_solver(A, b, c, &lp_barrier);
    lp_solver.run_solver();
    Solution sol = lp_solver.get_solution();
    return lp_solver.verify_solution(10e-5);
}

void test_sdp_solver() {
//    PolynomialSDP poly1 = envelopeProblem.generate_zero_polynomial();
//    poly1(0,0) = 1;
//    poly1(1, 0) = 1;
//    poly1(-0,1) = 1;
//    poly1(1, 1) = 1;
//    poly1(2,2) = -1;
//    poly1(1,1) = 1;
//
//    Polynomial poly11 = envelopeProblem.generate_zero_polynomial();
//    poly11(0,0) = 1;
//    poly11(1, 0) = -1;
//    poly11(-0,1) = -1;
//    poly11(1, 1) = 1;
//
//    envelopeProblem.print_polynomial(poly1);
//    envelopeProblem.add_polynomial(poly1);
//
//    envelopeProblem.print_polynomial(poly11);
//    envelopeProblem.add_polynomial(poly11);
//
//
//    PolynomialSDP p1 = Matrix::Zero(poly1.rows(), poly1.cols());
//    p1(1,1) = 10;
//    p1(0, 0) = -0;
//
//    PolynomialSDP p2 = Matrix::Zero(poly1.rows(), poly1.cols());
//    p2(0,0) = 10;
//    p2(0,1) = 10;
//    for (int i = 2; i <= max_degree; ++i) {
//        p2(i,i) = -.0001;
//    }
//    p2 = p2.eval() + p2.transpose().eval();
//    PolynomialSDP p3 = Matrix::Zero(poly1.rows(), poly1.cols());
//    p3(0,0) = 1;
//    p3(0,1) = -1;
//    p3 = p3.eval() + p3.transpose().eval();
////
//    envelopeProblem.add_polynomial(p1);
//    envelopeProblem.add_polynomial(p2);
//    envelopeProblem.add_polynomial(p3);
////
//    Instance instance = envelopeProblem.construct_SDP_instance();
//
//    std::cout << "Objectives Matrix: " << std::endl << envelopeProblem.get_objective_matrix() << std::endl;
////
//    NonSymmetricIPM sos_solver(instance);
//    sos_solver.run_solver();
//    envelopeProblem.print_solution(sos_solver.get_solution());
//    envelopeProblem.plot_polynomials_and_solution(sos_solver.get_solution());
}

