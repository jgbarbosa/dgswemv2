#ifndef CLASS_ELEMENT_H
#define CLASS_ELEMENT_H

class ELEMENT {
protected:
    unsigned int ID;

	unsigned int* neighbor_ID;
	unsigned char* boundary_type;

	double* nodal_coordinates_x;
	double* nodal_coordinates_y;
    
public:
	ELEMENT(int ID) : ID(ID) {}
	virtual ~ELEMENT() = default;

	virtual void ComputeInternalU(int) = 0;
	virtual void ComputeBoundaryU(int, int) = 0;

	virtual void CreateInterfaces() = 0;

	virtual double IntegrationInternalPhi(int, int) = 0;
	virtual double IntegrationInternalDPhiDX(int, int) = 0;
	virtual double IntegrationInternalDPhiDY(int, int) = 0;

	virtual double IntegrationBoundaryNX(int, int, int) = 0;
	virtual double IntegrationBoundaryNY(int, int, int) = 0;

	//virtual double test_against_phi(double f_at_gp[]) = 0;
	//virtual double test_against_dphidx(double f_at_gp[]) = 0;
	//virtual double test_against_dphidy(double f_at_gp[]) = 0;

    //virtual double* invert_mass_matrix(double f_bf[]) = 0;
};

#endif