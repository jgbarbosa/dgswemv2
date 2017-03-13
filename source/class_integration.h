#ifndef CLASS_INTEGRATION_H
#define CLASS_INTEGRATION_H

#include "general_definitions.h"
#include "integration_rules/integration_rules_area.h"
#include "integration_rules/integration_rules_line.h"

class INTEGRATION_1D {

private:
    int p;
    int number_gp;
    double* w;
    double* z;

public:
    INTEGRATION_1D(int, int);
    ~INTEGRATION_1D();

    int GetPolynomial();
    int GetNumberGP();
    double* GetWeight();
    double* GetZ();

private:
    void GaussLegendre();
};

class INTEGRATION_2D {
private: 
    int p;
    int number_gp;
    double* w;
    double* z1;
    double* z2;

public:
    INTEGRATION_2D(int, int);
    ~INTEGRATION_2D();

    int GetPolynomial();
    int GetNumberGP();
    double* GetWeight();
    double* GetZ1();
    double* GetZ2();

private:	
    void Dunavant();
};

#endif