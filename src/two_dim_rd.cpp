/**************************************************************************
 *   This file is part of TURING.                                         *
 *                                                                        *
 *   Author: Ivo Filot <ivo@ivofilot.nl>                                  *
 *                                                                        *
 *   TURING is free software:                                             *
 *   you can redistribute it and/or modify it under the terms of the      *
 *   GNU General Public License as published by the Free Software         *
 *   Foundation, either version 3 of the License, or (at your option)     *
 *   any later version.                                                   *
 *                                                                        *
 *   TURING is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty          *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.              *
 *   See the GNU General Public License for more details.                 *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program.  If not, see http://www.gnu.org/licenses/.  *
 *                                                                        *
 **************************************************************************/

#include "two_dim_rd.h"

/**
 * @brief      Constructs the object.
 *
 * @param[in]  _Da      Diffusion coefficient of compound A
 * @param[in]  _Db      Diffusion coefficient of compound B
 * @param[in]  _alpha   Alpha value in reaction equation
 * @param[in]  _beta    Beta value in reaction equation
 * @param[in]  _width   width of the system
 * @param[in]  _height  height of the system
 * @param[in]  _dx      size of the space interval
 * @param[in]  _dt      size of the time interval
 * @param[in]  _steps   number of frames
 * @param[in]  _tsteps  number of time steps when to write a frame
 */
TwoDimRD::TwoDimRD(double _Da, double _Db,
                   unsigned int _width, unsigned int _height,
                   double _dx, double _dt, unsigned int _steps, unsigned int _tsteps) :
    Da(_Da),
    Db(_Db),
    width(_width),
    height(_height),
    dx(_dx),
    dt(_dt),
    steps(_steps),
    tsteps(_tsteps) {

}

void TwoDimRD::set_reaction(ReactionSystem* _reaction_system) {
    this->reaction_system = std::unique_ptr<ReactionSystem>(_reaction_system);
}

/**
 * @brief      Perform time integration
 */
void TwoDimRD::time_integrate() {
    this->t = 0;

    for(int i : tq::trange(this->steps)) {
        for(unsigned int j=0; j<this->tsteps; j++) {
            this->update();
        }

        this->ta.push_back(this->a);
        this->tb.push_back(this->b);
    }

    // give newline after tqdm progress bar
    std::cout << std::endl;
}

/**
 * @brief      Write the current state of compound A to the file
 *
 * @param[in]  filename  The filename
 */
void TwoDimRD::write_state_to_file(const std::string& filename) {
    std::ofstream out(filename, std::ios::out | std::ios::binary | std::ios::trunc);

    typename MatrixXXd::Index rows, cols;

    // store width and height
    out.write((char*) (&this->width), sizeof(unsigned int) );
    out.write((char*) (&this->height), sizeof(unsigned int) );

    // store number of frames
    out.write((char*) (&this->steps), sizeof(unsigned int) );

    for(unsigned int i=0; i<this->ta.size(); i++) {
        const auto& a = this->ta[i];
        const auto& b = this->tb[i];

        // store a
        rows = a.rows();
        cols = a.cols();

        out.write((char*) a.data(), rows * cols * sizeof(typename MatrixXXd::Scalar) );

        // store b
        rows = b.rows();
        cols = b.cols();

        out.write((char*) b.data(), rows * cols * sizeof(typename MatrixXXd::Scalar) );
    }

    out.close();
}

/**
 * @brief      Initialize the system
 */
void TwoDimRD::init() {
    // initialize matrices with random values
    this->a = MatrixXXd::Zero(this->width, this->height);
    this->b = MatrixXXd::Zero(this->width, this->height);

    this->reaction_system->init(this->a, this->b);

    this->delta_a = MatrixXXd::Zero(this->width, this->height);
    this->delta_b = MatrixXXd::Zero(this->width, this->height);

    this->ta.push_back(this->a);
    this->tb.push_back(this->b);
}

/**
 * @brief      Perform a time-step
 */
void TwoDimRD::update() {
    // calculate laplacian
    if(this->pbc) {
        this->laplacian_2d_pbc(this->delta_a, this->a);
        this->laplacian_2d_pbc(this->delta_b, this->b);
    } else {
        this->laplacian_2d_zeroflux(this->delta_a, this->a);
        this->laplacian_2d_zeroflux(this->delta_b, this->b);
    }

    // multiply with diffusion coefficient
    this->delta_a *= this->Da;
    this->delta_b *= this->Db;

    // add reaction term
    this->add_reaction();

    // multiply with time step
    this->delta_a *= this->dt;
    this->delta_b *= this->dt;

    // add delta term to concentrations
    this->a += this->delta_a;
    this->b += this->delta_b;

    // update time step
    this->t += this->dt;
}

/**
 * @brief      Calculate Laplacian using central finite difference with periodic boundary conditions
 *
 * @param      delta_c  Concentration update matrix
 * @param      c        Current concentration matrix
 *
 * Note that this overwrites the current delta matrices!
 */
void TwoDimRD::laplacian_2d_pbc(MatrixXXd& delta_c, MatrixXXd& c) {
    const double idx2 = 1.0 / (this->dx * this->dx);

    #pragma omp parallel for schedule(static)
    for(int i=0; i<this->height; i++) {
        // indices
        unsigned i1;
        unsigned i2;

        if(i == 0) {
            i1 = this->height-1;
            i2 = i+1;
        } else if(i == (this->height-1)) {
            i1 = i-1;
            i2 = 0;
        } else {
            i1 = i-1;
            i2 = i+1;
        }

        // loop over x axis
        for(int j=0; j<this->width; j++) {
            // indices
            unsigned j1;
            unsigned j2;

            if(j == 0) {
                j1 = this->width-1;
                j2 = j+1;
            } else if(j == (this->width-1)) {
                j1 = j-1;
                j2 = 0;
            } else {
                j1 = j-1;
                j2 = j+1;
            }

            // calculate laplacian
            delta_c(i,j) = (-4.0 * c(i,j)
                                 + c(i1, j)
                                 + c(i2, j)
                                 + c(i, j1)
                                 + c(i, j2) ) * idx2;
        }
    }
}

/**
 * @brief      Calculate Laplacian using central finite difference with zero-flux boundaries
 *
 * @param      delta_c  Concentration update matrix
 * @param      c        Current concentration matrix
 *
 * Note that this overwrites the current delta matrices!
 */
void TwoDimRD::laplacian_2d_zeroflux(MatrixXXd& delta_c, MatrixXXd& c) {
    unsigned int height = c.rows();
    unsigned int width = c.cols();

    const double idx2 = 1.0 / (dx * dx);

    #pragma omp parallel for
    for(unsigned int i=0; i<height; i++) {
        for(unsigned int j=0; j<width; j++) {

            double ddx = 0;
            double ddy = 0;

            if(i == 0) {
                ddx = c(i+1,j) - c(i, j);
            } else if(i == (height - 1)) {
                ddx = c(i-1,j) - c(i, j);
            } else {
                ddx = (-2.0 * c(i,j) + c(i-1,j) + c(i+1,j));
            }

            if(j == 0) {
                ddy = c(i,j+1) - c(i, j);
            } else if(j == (width - 1)) {
                ddy = c(i,j-1) - c(i, j);
            } else {
                ddy = (-2.0 * c(i,j) + c(i,j-1) + c(i,j+1));
            }

            // calculate laplacian
            delta_c(i,j) = (ddx + ddy) * idx2;
        }
    }
}

/**
 * @brief      Calculate reaction term
 *
 * Add the value to the current delta matrices
 */
void TwoDimRD::add_reaction() {
    #pragma omp parallel for schedule(static)
    for(unsigned int i=0; i<this->height; i++) {
        for(unsigned int j=0; j<this->width; j++) {
            const double a = this->a(i,j);
            const double b = this->b(i,j);
            double ra = 0;
            double rb = 0;
            this->reaction_system->reaction(a, b, &ra, &rb);
            this->delta_a(i,j) += ra;
            this->delta_b(i,j) += rb;
        }
    }
}
