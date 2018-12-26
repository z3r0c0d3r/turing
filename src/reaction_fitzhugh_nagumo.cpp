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

#include "reaction_fitzhugh_nagumo.h"

ReactionFitzhughNagumo::ReactionFitzhughNagumo() {

}

void ReactionFitzhughNagumo::init(MatrixXXd& a, MatrixXXd& b) const {

}

void ReactionFitzhughNagumo::reaction(double a, double b, double *ra, double *rb) const {
    *ra = a - (a * a * a) - b + this->alpha;
    *rb = (a - b) * this->beta;
}

/**
 * @brief      Sets the parameters.
 *
 * @param[in]  params  The parameters
 */
void ReactionFitzhughNagumo::set_parameters(const std::string& params) {

}
