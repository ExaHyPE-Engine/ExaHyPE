// solves the exact riemann problem
// original code from bruce fryxell: https://cococubed.com/code_pages/exact_riemann.shtml

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <fstream>
#include <limits>

// solves shock tube equation
double f(double p4, double p1, double p5, double rho1, double rho5, double gamma)
{
    // local variables
    double z, c1, c5, gm1, gp1, g2, fact;

    z = (p4 / p5 - 1.0);
    c1 = std::sqrt(gamma * p1 / rho1);
    c5 = std::sqrt(gamma * p5 / rho5);

    gm1 = gamma - 1.0;
    gp1 = gamma + 1.0;
    g2 = 2.0 * gamma;

    fact = gm1 / g2 * (c5 / c1) * z / std::sqrt(1.0 + gp1 / g2 * z);
    fact = std::pow(1.0 - fact, g2 / gm1);

    return p1 * fact - p4;
}

int main(int argn, char **argc)
{
    if (argn > 2)
    {
        int npts = std::stoi(argc[1]); // number of cells per row/column
        double t = std::stod(argc[2]); // time at which solution is desired

        // declare variables
        int itmax, iter, i;
        std::vector<double> x(npts), rho(npts), u(npts), p(npts);
        double rhol, pl, ul, rhor, pr, ur, gamma, xi, xl, xr,
            rho1, p1, u1, rho5, p5, u5, p40, p41, f0, eps, f1, p4,
            error, z, c5, gm1, gp1, gmfac1, gmfac2, fact, u4, rho4, w,
            p3, u3, rho3, c1, c3, xsh, xcd, xft, xhd, dx;

        // set initial conditions

        // state at left of discontinuity
        rhol = 1.0;
        pl = 1.0;
        ul = 0.0;

        // state at right of discontinuity
        rhor = 0.125;
        pr = 0.1;
        ur = 0.0;

        //  equation of state
        gamma = 1.4;

        // location of discontinuity at t = 0
        xi = 0.5;

        // spatial interval over which to compute solution
        xl = 0.0;
        xr = 1.0;

        // begin solution
        rho1 = rhol;
        p1 = pl;
        u1 = ul;
        rho5 = rhor;
        p5 = pr;
        u5 = ur;

        // solve for post-shock pressure by secant method
        // initial guesses

        p40 = p1;
        p41 = p5;
        f0 = f(p40, p1, p5, rho1, rho5, gamma);

        // maximum number of iterations and maxium allowable relative error
        itmax = 200;
        eps = std::numeric_limits<double>::min();
        iter = 0;
        while (iter < itmax)
        {
            f1 = f(p41, p1, p5, rho1, rho5, gamma);
            if (f1 == f0)
                break;
            p4 = p41 - (p41 - p40) * f1 / (f1 - f0);
            error = std::abs(p4 - p41) / p41;
            if (error < eps)
                break;
            p40 = p41;
            p41 = p4;
            f0 = f1;
            iter++;
        }

        // compute post-shock density and velocity
        z = (p4 / p5 - 1.0);
        c5 = std::sqrt(gamma * p5 / rho5);

        gm1 = gamma - 1.0;
        gp1 = gamma + 1.0;
        gmfac1 = 0.5 * gm1 / gamma;
        gmfac2 = 0.5 * gp1 / gamma;

        fact = std::sqrt(1.0 + gmfac2 * z);

        u4 = c5 * z / (gamma * fact);
        rho4 = rho5 * (1.0 + gmfac2 * z) / (1.0 + gmfac1 * z);

        // shock speed
        w = c5 * fact;

        // compute values at foot of rarefaction
        p3 = p4;
        u3 = u4;
        rho3 = rho1 * std::pow(p3 / p1, 1.0 / gamma);

        // compute positions of waves
        c1 = sqrt(gamma * p1 / rho1);
        c3 = sqrt(gamma * p3 / rho3);

        xsh = xi + w * t;
        xcd = xi + u3 * t;
        xft = xi + (u3 - c3) * t;
        xhd = xi - c1 * t;

        // compute solution as a function of position
        dx = (xr - xl) / (npts - 1);
        for (int i = 0; i < npts; i++)
            x[i] = xl + dx * (i + 0.5);

        for (int i = 0; i < npts; i++)
        {
            if (x[i] < xhd)
            {
                rho[i] = rho1;
                p[i] = p1;
                u[i] = u1;
            }
            else if (x[i] < xft)
            {
                u[i] = 2.0 / gp1 * (c1 + (x[i] - xi) / t);
                fact = 1.0 - 0.5 * gm1 * u[i] / c1;
                rho[i] = rho1 * std::pow(fact, (2.0 / gm1));
                p[i] = p1 * std::pow(fact, 2.0 * gamma / gm1);
            }
            else if (x[i] < xcd)
            {
                rho[i] = rho3;
                p[i] = p3;
                u[i] = u3;
            }
            else if (x[i] < xsh)
            {
                rho[i] = rho4;
                p[i] = p4;
                u[i] = u4;
            }
            else
            {
                rho[i] = rho5;
                p[i] = p5;
                u[i] = u5;
            }
        }

        std::ofstream output("sod_analytical.csv");
        output << "x-momentum, density, energy\n";
        for (int i = 0; i < npts; i++)
        {
            double e = p[i] / (gamma - 1) + 0.5 * u[i] * u[i];
            output << rho[i] * u[i] << "," << rho[i] << "," << e << "\n";
        }
        output.close();
    }
    else
    {
        std::cout << "Error provide following details:\n1)number of cells per row/column \n2)time of solution\n";
    }
}
