
//@HEADER
// ************************************************************************
//
//               HPCG: Simple Conjugate Gradient Benchmark Code
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ************************************************************************
//@HEADER

// Changelog
//
// Version 0.3
// - Added timing of setup time for sparse MV
// - Corrected percentages reported for sparse MV with overhead
//
/////////////////////////////////////////////////////////////////////////

// Main routine of a program that calls the HPCG conjugate gradient
// solver to solve the problem, and then prints results.

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cassert>
#include <string>
#include <cmath>
#ifdef USING_MPI
#include <mpi.h> // If this routine is compiled with -DUSING_MPI then include mpi.h
#endif
#include "GenerateGeometry.hpp"
#include "GenerateProblem.hpp"
#include "OptimizeMatrix.hpp" // Also include this function
#include "WriteProblem.hpp"
#include "ReportResults.hpp"
#include "mytimer.hpp"
#include "spmv.hpp"
#include "ComputeResidual.hpp"
#include "CG.hpp"
#include "Geometry.hpp"
#include "SparseMatrix.hpp"

#undef DEBUG

int main(int argc, char *argv[]) {
    
    Geometry geom;
    SparseMatrix A;
    double *x, *b, *xexact;
    double norm, d;
    int ierr = 0;
    int i, j;
    int ione = 1;
    double times[8];
    double t7 = 0.0;
    int nx,ny,nz;
    
#ifdef USING_MPI
    
    MPI_Init(&argc, &argv);
    int size, rank; // Number of MPI processes, My process ID
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    //  if (size < 100) cout << "Process "<<rank<<" of "<<size<<" is alive." <<endl;
    
#else
    
    int size = 1; // Serial case (not using MPI)
    int rank = 0;
    
#endif
    
    
#ifdef DEBUG
    if (rank==0)
    {
        int junk = 0;
        cout << "Press enter to continue"<< endl;
        cin >> junk;
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
#endif
    
    
    if(argc!=4) {
        if (rank==0)
            cerr << "Usage:" << endl
            << argv[0] << " nx ny nz" << endl
            << "     where nx, ny and nz are the local sub-block dimensions, or" << endl;
        exit(1);
    }
    
    nx = atoi(argv[1]);
    ny = atoi(argv[2]);
    nz = atoi(argv[3]);
    GenerateGeometry(size, rank, nx, ny, nz, geom);
    GenerateProblem(geom, A, &x, &b, &xexact);
    //if (geom.size==1) WriteProblem(A, x, b, xexact);
    
#ifdef USING_MPI
    
    // Transform matrix indices from global to local values.
    // Define number of columns for the local matrix.
    
    t7 = mytimer(); OptimizeMatrix(geom, A);  t7 = mytimer() - t7;
    times[7] = t7;
    
#endif
    
    double t1 = mytimer();   // Initialize it (if needed)
    int niters = 0;
    double normr = 0.0;
    int maxIters = 50;
    int numberOfCgCalls = 1;
    double tolerance = 0.0; // Set tolerance to zero to make all runs do max_iter iterations
    for (int i=0; i< numberOfCgCalls; ++i) {
    	ierr = CG( geom, A, b, x, maxIters, tolerance, niters, normr, times);
    	if (ierr) cerr << "Error in call to CG: " << ierr << ".\n" << endl;
    	if (rank==0) cout << "Call [" << i << "] Residual [" << normr << "]" << endl;
    	for (int j=0; j< A.localNumberOfRows; ++j) x[j] = 0.0;
    }
    
    // Compute difference between known exact solution and computed solution
    // All processors are needed here.
#ifdef DEBUG
    double residual = 0;
    if ((ierr = compute_residual(A.localNumberOfRows, x, xexact, &residual)))
    cerr << "Error in call to compute_residual: " << ierr << ".\n" << endl;
    if (rank==0)
    cout << "Difference between computed and exact  = " << residual << ".\n" << endl;
#endif

    // Report results to YAML file
    ReportResults(geom, A, niters, normr, times);

    // Clean up
    destroyMatrix(A);
    delete [] x;
    delete [] b;
    delete [] xexact;
    
    // Finish up
#ifdef USING_MPI
    MPI_Finalize();
#endif
    return 0 ;
} 
