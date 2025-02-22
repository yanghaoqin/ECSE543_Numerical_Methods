/******************************************************************************/
/* Name: FDM.h                                                                */
/* Description: Finite Difference Method                                      */
/* Date: 2020/10/03                                                           */
/* Author: Raymond Yang                                                       */
/******************************************************************************/

#ifndef __FDM__
#define __FDM__

#include <math.h>
#include <chrono>
#include <limits>
#include "Matrix.h"
#include "Matrix_Solver.h" 

template <class T = double>
class FDM {
    private:
        int n_row;
        int n_col;
        bool uniform;
        Matrix<T> phi;  // node potential
        Matrix<bool> state;  // node state. [true->free | false->fixed].
        Matrix<T> h_lines, v_lines;  // horizontal and vertical grid lines

        // CG parameters
        Matrix<T> A_;
        Matrix<T> b_;
        Matrix<T> LUT_;

    public:
        // Uniform Spacing
        FDM(int n_row, int n_col, double h);
        FDM(double n_row, double n_col, double h);
        FDM(double h, Matrix<T> phi, Matrix<bool> state);
        // Non-Uniform Spacing
        FDM(int n_row, int n_col, double width, double height, double rate_x,
            double rate_y);
        FDM(Matrix<T> h_lines, Matrix<T> v_lines);
        FDM(Matrix<T> phi, Matrix<bool> state, Matrix<T> h_lines, 
            Matrix<T> v_lines);
        ~FDM();

        // print mesh
        void show() const;
        void show_phi() const;
        void show_state() const;
        void show_lines() const;

        // deep copy
        FDM<T> deep_copy();

        // getters and setters
        T get_n_row();
        T get_n_col();
        T get_h();
        T get_phi(int row, int col);
        Matrix<T> get_phi();
        bool get_state(int row, int col);
        Matrix<bool> get_state();
        Matrix<T> get_h_lines();
        Matrix<T> get_v_lines();
        void free(int row, int col);
        void free(int row1, int row2, int col1, int col2);
        void fix(int row, int col);
        void fix(int row1, int row2, int col1, int col2);
        void set(int row1, int row2, int col1, int col2, T phi, bool state);
        void set_phi(int row, int col, T phi);
        void set_phi(int row1, int row2, int col1, int col2, T phi);
        void set_phi(int row, int col, Matrix<T> phi);
        Matrix<T> get_A();
        Matrix<T> get_b();
        Matrix<T> get_LUT();
        void set_A(Matrix<T> A);
        void set_b(Matrix<T> b);
        void set_LUT(Matrix<T> LUT);

        
        // check if mesh has uniform node spacing
        bool is_uniform() const;

        // Solvers
        void SOR(double omega, double tol);
        void jacobi(double tol);
        void CG(int itr = -1, bool VERBOSE = false);

        // Post Process
        double calc_W(Matrix<T> S_con);

        // public variables
        int num_itr = 0;
        double max_delta = 0;
        double duration = 0;
};

// -----------------------------------------------------------------------------
// Constructor & Destructor
// -----------------------------------------------------------------------------

/*  
    Constructor for FDM with Uniform Node Spacing  
    Initialized to 0 potential and free nodes

    n_row: number of nodes in row
    n_col: number of nodes in column
    h: node spacing (uniform)    
*/
template <class T>
FDM<T>::FDM(int n_row, int n_col, double h): n_row(n_row), n_col(n_col){
    if(n_row == 0 || n_col == 0){
        throw "FDM::FDM Warning: total 0 nodes, empty mesh.";
    }
    this->uniform = true;
    this->phi = Matrix<T>(n_row, n_col);
    this->state = Matrix<bool>(n_row, n_col);
    this->h_lines = Matrix<T>(1, n_col);
    this->v_lines = Matrix<T>(1, n_row);
    for(int i = 0; i < n_col; i++){
        this->h_lines.set(0, i, i*h);
    }
    for(int i = 0; i < n_row; i++){
        this->v_lines.set(0, i, i*h);
    }
    return;
}

/*  
    Constructor for FDM with Uniform Node Spacing  
    Initialized to 0 potential and free nodes

    n_row: number of nodes in row
    n_col: number of nodes in column
    h: node spacing (uniform)    
*/
template <class T>
FDM<T>::FDM(double n_row, double n_col, double h): n_row(n_row), n_col(n_col){
    if(n_row == 0 || n_col == 0){
        throw "FDM::FDM Error: total 0 nodes, empty mesh.";
    } else if((int)(n_row) != n_row || (int)(n_col) != n_col) {
        std::cout << "FDM::FDM Warning: number of nodes is not an integer,"
            << "automatic conversion to (int) type." << std::endl;
    } else {}

    n_row = (int)(n_row);
    n_col = (int)(n_col);

    this->uniform = true;
    this->phi = Matrix<T>(n_row, n_col);
    this->state = Matrix<bool>(n_row, n_col, 1);
    this->h_lines = Matrix<T>(1, n_col);
    this->v_lines = Matrix<T>(1, n_row);
    for(int i = 0; i < n_col; i++){
        this->h_lines.set(0, i, i*h);
    }
    for(int i = 0; i < n_row; i++){
        this->v_lines.set(0, i, i*h);
    }
    return;
}

/*  
    Constructor for FDM with Uniform Node Spacing  

    h: node spacing (uniform)  
    phi: node potential matrix
    state: node state matrix  
*/
template <class T>
FDM<T>::FDM(double h, Matrix<T> phi, Matrix<bool> state): 
    phi(phi), state(state){
    this->uniform = true;
    this->n_row = phi.get_n_row();
    this->n_col = phi.get_n_col();
    if(n_row == 0 || n_col == 0){
        throw "FDM::FDM Warning: total 0 nodes, empty mesh.";
    }

    this->h_lines = Matrix<T>(1, this->n_col);
    this->v_lines = Matrix<T>(1, this->n_row);
    for(int i = 0; i < this->n_col; i++){
        this->h_lines.set(0, i, i*h); 
    }
    for(int i = 0; i < this->n_row; i++){
        this->v_lines.set(0, i, i*h);
    }
    return;
}

/*  
    Constructor for FDM with exponential non-uniform node spacing
    Initialized to 0 potential and free nodes

    n_row: number of nodes in row
    n_col: number of nodes in column
    width: physical horizontal width
    height: physical vertical height
    rate_x: horizontal exponential rate
    rate_y: vertical exponential rate
*/
template <class T>
FDM<T>::FDM(int n_row, int n_col, double width, double height, double rate_x,
    double rate_y): n_row(n_row), n_col(n_col), uniform(false) {
    this->h_lines = Matrix<T>(1, n_row);
    this->v_lines = Matrix<T>(1, n_row);
    double delta_x = width/((1 - pow((double)(1)/rate_x, n_row-1)) 
        /(1 - (double)(1)/rate_x));
    double delta_y = height/((1 - pow((double)(1)/rate_y, n_col-1)) 
        /(1 - (double)(1)/rate_y));
    for(int i = 1; i < n_row; i++){
        this->v_lines.set(0,i,this->v_lines.get(0,i-1) 
            + delta_x*pow((1/rate_x), i-1));
    }
    for(int i = 1; i < n_col; i++){
        this->h_lines.set(0,i,this->h_lines.get(0,i-1) 
            + delta_y*pow((1/rate_y), i-1));
    }
    this->phi = Matrix<T>(n_row, n_col);
    this->state = Matrix<bool>(n_row, n_col);
    return;
}

/*  
    Constructor for FDM with non-uniform node spacing
    Initialized to 0 potential and free nodes

    h_lines: horizontal grid line positions
    v_lines: vertical grid line positions
*/
template <class T>
FDM<T>::FDM(Matrix<T> h_lines, Matrix<T> v_lines): 
    h_lines(h_lines), v_lines(v_lines) {
    this->uniform = false;
    this->n_row = h_lines.get_n_col();
    this->n_col = v_lines.get_n_col();
    if(this->n_row == 0 || this->n_col == 0){
        throw "FDM::FDM Warning: total 0 nodes, empty mesh.";
    }
    for(int i = 1; i < this->n_row; i++){
        if((this->h_lines.get(i) - this->h_lines.get(i-1)) 
            > (this->h_lines.get(this->n_row-1) - this->h_lines.get(0))){
                throw "FDM::FDM Error: vertical node spacing error.";
        }
    }
    for(int i = 0; i < this->n_col; i++){
        if((this->v_lines.get(i) - this->v_lines.get(i-1)) 
            > (this->v_lines.get(this->n_row-1) - this->v_lines.get(0))){
                throw "FDM::FDM Error: horizontal node spacing error.";
        }
    }
    this->phi = Matrix<T>(this->n_row, this->n_col);
    this->state = Matrix<bool>(this->n_row, this->n_col, 1);
    return;
}

/*  
    Constructor for FDM with non-uniform node spacing 

    phi: node potential matrix
    state: node state matrix
    h_lines: horizontal grid line positions
    v_lines: vertical grid line positions 
*/
template <class T>
FDM<T>::FDM(Matrix<T> phi, Matrix<bool> state, Matrix<T> h_lines, 
    Matrix<T> v_lines): phi(phi), state(state), h_lines(h_lines), 
    v_lines(v_lines) {
    this->uniform = false;
    this->n_row = phi.get_n_row();
    this->n_col = phi.get_n_col();
    if(n_row == 0 || n_col == 0){
        throw "FDM::FDM Warning: total 0 nodes, empty mesh.";
    }
    return;
}

/*  Default destructor for FDM  */
template <class T>
FDM<T>::~FDM(){}

// -----------------------------------------------------------------------------
// Member Functions
// -----------------------------------------------------------------------------

/*  Print the FD mesh phi and state in terminal  */
template <class T>
void FDM<T>::show() const{
    std::cout << "FD Mesh Potential:" << std::endl;
    this->show_phi();
    std::cout << "FD Mesh State: [1=Free, 0=Fixed]" << std::endl;
    this->show_state();
    std::cout << "Horizontal Grid Lines: " << std::endl;
    this->h_lines.show();
    std::cout << "Vertical Grid Lines: " << std::endl;
    this->v_lines.show();
    return;
}

/*  Print the FD mesh potential in terminal  */
template <class T>
void FDM<T>::show_phi() const{
    if(this->n_row == 0 && this->n_col == 0){
        std::cout << "0 [ ]" << std::endl;
    }
    for(int i = 0; i < this->n_row; i++){
        std::cout << i << " [ ";
        for(int j = 0; j < this->n_col; j++){
            std::cout << this->phi.get(i,j) << ",";
        }
        std::cout << " ]" << std::endl;
    }
    return;
}

/*  Print the FD mesh states in terminal  */
template <class T>
void FDM<T>::show_state() const{
    if(this->n_row == 0 && this->n_col == 0){
        std::cout << "0 [ ]" << std::endl;
    }
    for(int i = 0; i < this->n_row; i++){
        std::cout << i << " [ ";
        for(int j = 0; j < this->n_col; j++){
            std::cout << this->state.get(i,j) << ",";
        }
        std::cout << " ]" << std::endl;
    }
    return;
}

/*  Print the FD mesh grid lines in terminal  */
template <class T>
void FDM<T>::show_lines() const{
    std::cout << "Horizontal Grid Lines: " << std::endl;
    this->h_lines.show();
    std::cout << "Vertical Grid Lines: " << std::endl;
    this->v_lines.show();
    return;
}

/*  Get a deep copy of the FDM  */
template <class T>
FDM<T> FDM<T>::deep_copy(){
    FDM<T> new_mesh(this->h, this->phi, this->state);
    new_mesh.A_ = this->A_;
    new_mesh.b_ = this->A_;
    new_mesh.LUT_ = this->LUT_;
    return new_mesh;
}

/*  Get number of rows  */
template <class T>
T FDM<T>::get_n_row(){
    return this->n_row;
}

/*  Get number of columns  */
template <class T>
T FDM<T>::get_n_col(){
    return this->n_col;
}

/*  Get node spacing (uniform case only)  */
template <class T>
T FDM<T>::get_h(){
    return this->h_lines.get(1) - this->h_lines.get(0);
}

/*  Get node potential at (row,col)  */
template <class T>
T FDM<T>::get_phi(int row, int col){
    return this->phi.get(row, col);
}

/*  Get the node potential phi matrix  */
template <class T>
Matrix<T> FDM<T>::get_phi(){
    return this->phi;
}

/*  Get node state at (row,col)  */
template <class T>
bool FDM<T>::get_state(int row, int col){
    return this->state.get(row, col);
}

/*  Get the node potential phi matrix  */
template <class T>
Matrix<bool> FDM<T>::get_state(){
    return this->state;
}

/*  Get the horizontal line coordinates  */
template <class T>
Matrix<T> FDM<T>::get_h_lines(){
    return this->h_lines;
}

/*  Get the vertical line coordinates  */
template <class T>
Matrix<T> FDM<T>::get_v_lines(){
    return this->v_lines;
}

/*  Check if node spacing is uniform  */
template <class T>
bool FDM<T>::is_uniform() const {
    return this->uniform;
}

/*  Set the node potential and node state of part of the array  */
template <class T>
void FDM<T>::set(int row1, int row2, int col1, int col2, T phi, bool state){
    this->set_phi(row1, row2, col1, col2, phi);
    if(state){
        this->free(row1, row2, col1, col2);
    } else {
        this->fix(row1, row2, col1, col2);
    }
    return;
}

/*  Set the node potential for node (row,col)  */
template <class T>
void FDM<T>::set_phi(int row, int col, T phi){
    this->phi.set(row, col, phi);
    return;
}

/*  Set the same node potential for nodes (row1:row2, col1:col2) */
template <class T>
void FDM<T>::set_phi(int row1, int row2, int col1, int col2, T phi){
    this->phi.set(row1, row2, col1, col2, phi);
    return;
}

/*  Set the same node potential for nodes (row1:row2, col1:col2) */
template <class T>
void FDM<T>::set_phi(int row, int col, Matrix<T> phi){
    this->phi.set(row, col, phi);
    return;
}

/*  Set the state of node (row,col) to free  */
template <class T>
void FDM<T>::free(int row, int col){
    this->state.set(row, col, true);
    return;
}

/*  Free nodes (row1:row2, col1:col2) */
template <class T>
void FDM<T>::free(int row1, int row2, int col1, int col2){
    for(int row = row1; row <= row2; row++){
        for(int col = col1; col <= col2; col++){
            this->free(row, col);
        }
    }
    return;
}

/*  Set the state of node (row,col) to fixed  */
template <class T>
void FDM<T>::fix(int row, int col){
    this->state.set(row, col, false);
    return;
}

/*  Free nodes (row1:row2, col1:col2) */
template <class T>
void FDM<T>::fix(int row1, int row2, int col1, int col2){
    for(int row = row1; row <= row2; row++){
        for(int col = col1; col <= col2; col++){
            this->fix(row, col);
        }
    }
    return;
}

/*  Get A matrix  */
template <class T>
Matrix<T> FDM<T>::get_A(){
    return this->A_;
}

/*  Get b matrix  */
template <class T>
Matrix<T> FDM<T>::get_b(){
    return this->b_;
}

/*  Get LUT  */
template <class T>
Matrix<T> FDM<T>::get_LUT(){
    return this->LUT_;
}

/*  Set A matrix  */
template <class T>
void FDM<T>::set_A(Matrix<T> A){
    this->A_ = A;
    return;
}

/*  Set b matrix  */
template <class T>
void FDM<T>::set_b(Matrix<T> b){
    this->b_ = b;
    return;
}

/*  Set LUT matrix  */
template <class T>
void FDM<T>::set_LUT(Matrix<T> LUT){
    this->LUT_ = LUT;
    return;
}

/*  
    Successive Over-Relaxation.
    Iterative solver for electrostatics.
    
    omega: relaxation parameter
    tol: tolerance for stop condition 
*/
template <class T>
void FDM<T>::SOR(double omega, double tol){
    this->max_delta = 0;
    this->num_itr = 0;
    this->duration = 0;
    double max_delta;
    int count = 0;
    auto tic = std::chrono::high_resolution_clock::now();
    if(this->uniform == true){
        do{
            max_delta = 0;
            for(int i = 0; i < this->n_row; i++){
                for(int j = 0; j < this->n_col; j++){
                    if(this->get_state(i,j) == false){
                        // Dirichlet boundary
                        continue;
                    } else {
                        // Neumann boundary
                        double new_val;
                        if(i == 0){
                            new_val = (1 - omega)*this->get_phi(i,j) 
                                + (omega/4)*(2*this->get_phi(i+1,j) 
                                + this->get_phi(i,j-1) + this->get_phi(i,j+1));
                        } else if(i == this->n_row - 1) {
                            new_val = (1 - omega)*this->get_phi(i,j) 
                                + (omega/4)*(2*this->get_phi(i-1,j) 
                                + this->get_phi(i,j-1) + this->get_phi(i,j+1));
                        } else if(j == 0) {
                            new_val = (1 - omega)*this->get_phi(i,j) 
                                + (omega/4)*(2*this->get_phi(i,j+1) 
                                + this->get_phi(i-1,j) + this->get_phi(i+1,j));
                        } else if(j == this->n_col - 1) {
                            new_val = (1 - omega)*this->get_phi(i,j) 
                                + (omega/4)*(2*this->get_phi(i,j-1) 
                                + this->get_phi(i-1,j) + this->get_phi(i+1,j));
                        } else {
                            // General node
                            new_val = (1 - omega)*this->get_phi(i,j) 
                                + (omega/4)*(this->get_phi(i-1,j) 
                                + this->get_phi(i,j-1) + this->get_phi(i+1,j) 
                                + this->get_phi(i,j+1));
                        }
                        // update max_delta
                        double delta = std::abs(new_val - this->get_phi(i,j));
                        max_delta = (max_delta < delta) ? delta : max_delta;
                        this->set_phi(i, j, new_val);
                    }
                }
            }
            count++;

            // EXCEPTION CONDITIONS
            if(count > 1e7){
                throw "FDM::SOR Error: Solver failed to converge. ITER > 1e7";
            }
            auto toc_tmp = std::chrono::high_resolution_clock::now();
            if(std::chrono::duration
                <double, std::milli>(toc_tmp - tic).count()/1e3 > 10*60){
                throw "FDM::SOR Error: Solver failed to converge. TIME > 10min";
            }
            if(max_delta > 1e6){
                throw "FDM::SOR Error: Solver failed to converge. DELTA > 1e6";
            }
        } while(max_delta > tol);
    } else {  // Non-Uniform Mesh
        double a1, a2, b1, b2;
        do{
            max_delta = 0;
            for(int i = 0; i < this->n_row; i++){
                for(int j = 0; j < this->n_col; j++){
                    if(this->get_state(i,j) == false){
                        // Dirichlet boundary
                        continue;
                    } else {
                        // Neumann boundary
                        double new_val;
                        if(i == 0){
                            a1 = this->v_lines.get(j) - this->v_lines.get(j-1);
                            a2 = this->v_lines.get(j+1) - this->v_lines.get(j);
                            b2 = this->h_lines.get(i+1) - this->h_lines.get(i);
                            new_val = (1 - omega)*this->get_phi(i,j) 
                                + (omega)*((this->get_phi(i,j-1)/(a1*(a1+a2)) 
                                + this->get_phi(i,j+1)/(a2*(a1+a2))
                                + this->get_phi(i+1,j)/(b2*b2)))
                                /(1/(a1*a2) + 1/(b2*b2));
                        } else if(i == this->n_row - 1) {
                            a1 = this->v_lines.get(j) - this->v_lines.get(j-1);
                            a2 = this->v_lines.get(j+1) - this->v_lines.get(j);
                            b1 = this->h_lines.get(i) - this->h_lines.get(i-1);
                            new_val = (1 - omega)*this->get_phi(i,j) 
                                + (omega)*(this->get_phi(i,j-1)/(a1*(a1+a2)) 
                                + this->get_phi(i,j+1)/(a2*(a1+a2))
                                + this->get_phi(i-1,j)/(b1*b1))
                                /(1/(a1*a2) + 1/(b1*b1));
                        } else if(j == 0) {
                            a2 = this->v_lines.get(j+1) - this->v_lines.get(j);
                            b1 = this->h_lines.get(i) - this->h_lines.get(i-1);
                            b2 = this->h_lines.get(i+1) - this->h_lines.get(i);
                            new_val = (1 - omega)*this->get_phi(i,j) 
                                + (omega)*(this->get_phi(i,j+1)/(a2*a2)
                                + this->get_phi(i-1,j)/(b1*(b1+b2))
                                + this->get_phi(i+1,j)/(b2*(b1+b2)))
                                /(1/(a2*a2) + 1/(b1*b2));
                        } else if(j == this->n_col - 1) {
                            a1 = this->v_lines.get(j) - this->v_lines.get(j-1);
                            b1 = this->h_lines.get(i) - this->h_lines.get(i-1);
                            b2 = this->h_lines.get(i+1) - this->h_lines.get(i);
                            new_val = (1 - omega)*this->get_phi(i,j) 
                                + (omega)*(this->get_phi(i,j-1)/(a1*a1) 
                                + this->get_phi(i-1,j)/(b1*(b1+b2))
                                + this->get_phi(i+1,j)/(b2*(b1+b2)))
                                /(1/(a1*a1) + 1/(b1*b2));
                        } else {
                            // General node
                            a1 = this->v_lines.get(j) - this->v_lines.get(j-1);
                            a2 = this->v_lines.get(j+1) - this->v_lines.get(j);
                            b1 = this->h_lines.get(i) - this->h_lines.get(i-1);
                            b2 = this->h_lines.get(i+1) - this->h_lines.get(i);
                            new_val = (1 - omega)*this->get_phi(i,j) 
                                + (omega)*(this->get_phi(i,j-1)/(a1*(a1+a2)) 
                                + this->get_phi(i,j+1)/(a2*(a1+a2))
                                + this->get_phi(i-1,j)/(b1*(b1+b2))
                                + this->get_phi(i+1,j)/(b2*(b1+b2)))
                                /(1/(a1*a2) + 1/(b1*b2));
                        }
                        // update max_delta
                        double delta = std::abs(new_val - this->get_phi(i,j));
                        max_delta = (max_delta < delta) ? delta : max_delta;
                        this->set_phi(i, j, new_val);
                    }
                }
            }
            count++;

            // EXCEPTION CONDITIONS
            if(count > 1e7){
                throw "FDM::SOR Error: Solver failed to converge. ITER > 1e7";
            }
            auto toc_tmp = std::chrono::high_resolution_clock::now();
            if(std::chrono::duration
                <double, std::milli>(toc_tmp - tic).count()/1e3 > 10*60){
                throw "FDM::SOR Error: Solver failed to converge. TIME > 10min";
            }
            if(max_delta > 1e6){
                throw "FDM::SOR Error: Solver failed to converge. DELTA > 1e6";
            }
        } while(max_delta > tol);

    }

    auto toc = std::chrono::high_resolution_clock::now();
    this->max_delta = max_delta;
    this->num_itr = count;
    this->duration = std::chrono::duration
        <double, std::milli>(toc - tic).count()/1e3;
    return;
}

/*  
    Jacobi Method.
    Iterative solver for electrostatics.

    tol: tolerance for stop condition 
*/
template <class T>
void FDM<T>::jacobi(double tol){
    this->max_delta = 0;
    this->num_itr = 0;
    this->duration = 0;
    double max_delta;
    int count = 0;
    Matrix<T> past;
    auto tic = std::chrono::high_resolution_clock::now();
    do{
        max_delta = 0;
        past = this->get_phi();
        for(int i = 0; i < this->n_row; i++){
            for(int j = 0; j < this->n_col; j++){
                if(this->get_state(i,j) == false){
                    // Dirichlet boundary
                    continue;
                } else {
                    // Neumann boundary
                    double new_val;
                    if(i == 0){
                        new_val = (2*past.get(i+1,j) + past.get(i,j-1) 
                            + past.get(i,j+1))/4;
                    } else if(i == this->n_row - 1) {
                        new_val = (2*past.get(i-1,j) + past.get(i,j-1) 
                            + past.get(i,j+1))/4;
                    } else if(j == 0) {
                        new_val = (2*past.get(i,j+1) + past.get(i-1,j) 
                            + past.get(i+1,j))/4;
                    } else if(j == this->n_col - 1) {
                        new_val = (2*past.get(i,j-1) + past.get(i-1,j) 
                            + past.get(i+1,j))/4;
                    } else {
                        // General node
                        new_val = (past.get(i-1,j) + past.get(i,j-1) 
                            + past.get(i+1,j) + past.get(i,j+1))/4;
                    }
                    // update max_delta
                    double delta = std::abs(new_val - past.get(i,j));
                    max_delta = (max_delta < delta) ? delta : max_delta;
                    this->set_phi(i, j, new_val);
                }
            }
        }
        count++;

        // EXCEPTION CONDITIONS
        if(count > 1e7){
            throw "FDM::jacobi Error: Solver failed to converge. ITER > 1e7";
        }
        auto toc_tmp = std::chrono::high_resolution_clock::now();
        if(std::chrono::duration
            <double, std::milli>(toc_tmp - tic).count()/1e3 > 10*60){
            throw "FDM::jacobi Error: Solver failed to converge. TIME > 10min";
        }
        if(max_delta > 1e6){
            throw "FDM::jacobi Error: Solver failed to converge. DELTA > 1e6";
        }
    } while(max_delta > tol);

    auto toc = std::chrono::high_resolution_clock::now();
    this->max_delta = max_delta;
    this->num_itr = count;
    this->duration = std::chrono::duration
        <double, std::milli>(toc - tic).count()/1e3;
    return;
}

/*  Compute the total energy W = E/e0 over the mesh */
template <class T>
double FDM<T>::calc_W(Matrix<T> S_con){
    int e_nodes = S_con.get_n_row();
    Matrix<T> U_con(4,1);
    double W = 0;
    for(int row = 0; row < this->n_row - 1; row++){
        for(int col = 0; col < this->n_col - 1; col++){
            if(this->state.get(row, col) + this->state.get(row + 1, col)
                + this->state.get(row, col + 1) 
                + this->state.get(row + 1, col + 1) > 0){
                U_con.set(0, 0, this->phi.get(row + 1, col));
                U_con.set(1, 0, this->phi.get(row, col));
                U_con.set(2, 0, this->phi.get(row, col + 1));
                U_con.set(3, 0, this->phi.get(row + 1, col + 1));
                W = W + ((0.5)*(transpose(U_con)*S_con*U_con)).get(0);
            }
        }
    }
    return W;
}

/*  
    Setup A, b, and LUT for the conjugate gradient (unpreconditioned) solver
    Call CG solver

    tol: tolerance for stop condition 
*/
template <class T>
void FDM<T>::CG(int itr, bool VERBOSE){
    // setup look-up-table
    Matrix<T> LUT_CG(this->n_row, this->n_col);  // IDs for free nodes
    int num_free_nodes = 0;
    for(int i = 0; i < this->n_row * this->n_col; i++){
        if(this->state.get(i) == true){
            LUT_CG.set(i, num_free_nodes);
            num_free_nodes++;
        } else {
            LUT_CG.set(i, -1);
        }
    }
    // setup A and b matrices
    Matrix<T> A_CG = -4.0*Matrix<>::identity_mat(num_free_nodes);
    Matrix<T> b_CG(num_free_nodes, 1);
    auto check = [&](int row, int col) -> bool {
        return this->get_state(row, col);
    };
    auto fixed_phi = [&](int row, int col) -> double {
        return (int)(!check(row, col))*(this->get_phi(row, col));
    };
    for(int row = 0; row < this->n_row; row++){
        for(int col = 0; col < this->n_col; col++){
            auto smart_set = [&, col, row](int row_, int col_, int factor){
                if(LUT_CG.get(row_, col_) >= 0){
                    A_CG.set(LUT_CG.get(row, col), LUT_CG.get(row_, col_), 
                        factor*check(row_, col_));
                }
                return;
            };
            if(this->state.get(row, col) == true){
                double sum = 0;
                if(row == 0){
                    sum -= 2*fixed_phi(row + 1, col) + fixed_phi(row, col - 1)
                        + fixed_phi(row, col + 1);
                    smart_set(row + 1, col, 2); 
                    smart_set(row, col - 1, 1);
                    smart_set(row, col + 1, 1); 
                } else if(row == this->n_row - 1) {
                    sum -= 2*fixed_phi(row - 1, col) + fixed_phi(row, col - 1)
                        + fixed_phi(row, col + 1);
                    smart_set(row - 1, col, 2); 
                    smart_set(row, col - 1, 1);
                    smart_set(row, col + 1, 1);
                } else if(col == 0) {
                    sum -= fixed_phi(row - 1, col) + fixed_phi(row + 1, col)
                        + 2*fixed_phi(row, col + 1);
                    smart_set(row, col + 1, 2); 
                    smart_set(row + 1, col, 1);
                    smart_set(row - 1, col, 1);
                } else if(col == this->n_col - 1) {
                    sum -= fixed_phi(row - 1, col) + fixed_phi(row + 1, col)
                        + 2*fixed_phi(row, col - 1);
                    smart_set(row, col - 1, 2); 
                    smart_set(row + 1, col, 1);
                    smart_set(row - 1, col, 1);
                } else {
                    sum -= fixed_phi(row - 1, col) + fixed_phi(row + 1, col)
                        + fixed_phi(row, col - 1) + fixed_phi(row, col + 1);
                    smart_set(row + 1, col, 1); 
                    smart_set(row - 1, col, 1); 
                    smart_set(row, col - 1, 1);
                    smart_set(row, col + 1, 1);
                }
                b_CG.set(LUT_CG.get(row, col), sum);
            }
        }
    }

    // convert A to SSPD
    this->A_ = transpose(A_CG) * A_CG;
    this->b_ = transpose(A_CG) * b_CG;
    this->LUT_ = LUT_CG;

    Matrix<> x = Matrix_Solver::CG_solve(this->A_, this->b_, itr);

    for(int row = 0; row < this->n_row; row++){
        for(int col = 0; col < this->n_col; col++){
            if(check(row, col) == true){
                this->set_phi(row, col, x.get(LUT_CG.get(row, col)));
            }
        }
    }

    if(VERBOSE){
        std::cout << "a)" << std::endl;
        // Attempt Cholesky decomposition on original A_CG matrix
        try{
            Matrix<> temp(A_CG.get_n_row(), A_CG.get_n_row());
            Matrix_Solver::cholesky(A_CG, &temp);
            std::cout << "cholesky decomposition successful! \n" << std::endl;
        }catch(const char* msg){
            std::cout << msg << std::endl;
        }

        std::cout << "\nb)" << std::endl;
        // Attempt Cholesky decomposition on new A_ matrix
        try{
            Matrix<> temp(this->A_.get_n_row(), this->A_.get_n_row());
            Matrix_Solver::cholesky(this->A_, &temp);
            std::cout << "cholesky decomposition successful! \n" << std::endl;
            temp.show();
        }catch(const char* msg){
            std::cout << msg << std::endl;
        }

        Matrix<> x2(x.get_n_row(), x.get_n_col()); 
        Matrix_Solver::cholesky_solve(this->A_, this->b_, &x2);
        std::cout << "\nConjugate Gradient Solution Vector Results" 
            << std::endl;
        x.show();

        std::cout << "\nCholesky Decomposition Solution Vector Results" 
            << std::endl;
        x2.show();
        
        std::cout << "\nd)" << std::endl;
        std::cout << "With CG: V(0.06,0.04) = " << x.get(LUT_CG.get(2, 3)) 
            << std::endl;

        std::cout << "With CD: V(0.06,0.04) = " << x2.get(LUT_CG.get(2, 3)) 
            << std::endl;
    }

    return;
}



#endif




