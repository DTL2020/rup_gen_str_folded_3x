// rup_gen.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "stdio.h"
#include <vector>
#include "math.h"

#include "vec3.h"
#include "quat.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define rad2deg(x) (x*180.0/M_PI)
#define deg2rad(x) (x*M_PI/180.0)

#include "matfunc.h"

struct Face {
	int v1;
	int v2;
	int v3;
};

// General model params

#define NUM_VERTEX_IN_CIRCLE 100 // one for all diameters define for now, better to do depending on diameter
#define NUM_STEPS_IN_RUPOR1 50 // length in 10ths of mm (centimeter) ?
#define NUM_STEPS_IN_RUPOR2 50 // length in 10ths of mm (centimeter) ?
#define NUM_STEPS_IN_RUPOR3 50 // length in 10ths of mm (centimeter) ?
#define SPECIAL_BEGIN 2
#define SPECIAL_END 2
float fRadius0 = 1.925f;// 0.3f; // start rupor radius. in centimeters.
float fAxisCoordStep = 1.0f; // rupor axis coord step in centimeters
float fBeta=0.024f; // beta exp param 
float fThickness = 0.1f; // spacing between inner and outer surfaces
float fThicknessBegin = 0.2f; // spacing between inner and outer surfaces
float fThicknessEnd = 0.2f; // spacing between inner and outer surfaces

// generate circle of vertices around given origin point rotated to given angle
void CircleGen(std::vector<vec3>& vVerts, vec3 vOrigin, float xAng, float yAng, float zAng, float fRadius)
{
    for (int i = 0; i < NUM_VERTEX_IN_CIRCLE; i++)
    {
		vec3 Vert;
		Vert.x = fRadius;
		Vert.y = 0;
		Vert.z = 0;

		float fRot = ((float)i * 360.0f)/ NUM_VERTEX_IN_CIRCLE; // in degrees or 

		// rotate around zero, Z-axis
		Vert = rotatept(Vert, deg2rad(fRot), vec3(0.0, 0.0, 1.0).normalized());

		// rotate around zero, X-axis
		Vert = rotatept(Vert, deg2rad(xAng), vec3(1.0, 0.0, 0.0).normalized());

		//move to origin
		Vert.x += vOrigin.x;
		Vert.y += vOrigin.y;
		Vert.z += vOrigin.z;

		vVerts.push_back(Vert);

    }
}

// connect 2 circles with faces
void CircleConnect(std::vector<Face>& vFaces, int iStartFirstCirc, int iStartSecondCirc)
{
	Face F1;
	Face F2;

	int iLastVertNum = iStartSecondCirc + 1;
	int iStartFirst = iStartFirstCirc;
	int iStartSecond = iStartSecondCirc;
	// connect by dual-triangles (quads), num of quads = num of verts in circle
	for (int i = 0; i < NUM_VERTEX_IN_CIRCLE - 1; i++) // all quads except last
	{
		// first triangle of quad

		F1.v1 = iStartFirst;
		F1.v2 = iStartSecond;
		F1.v3 = iLastVertNum;

		// second triangle of quad
		F2.v1 = iStartFirst + 1;
		F2.v2 = iStartFirst;
		F2.v3 = iLastVertNum;

		vFaces.push_back(F1);
		vFaces.push_back(F2);

		iLastVertNum++;
		iStartFirst++;
		iStartSecond++;
	}

	// last quad
	F1.v1 = iStartFirst;
	F1.v2 = iStartSecond;
	F1.v3 = iStartSecondCirc;

	F2.v1 = iStartFirstCirc;
	F2.v2 = iStartFirst;
	F2.v3 = iStartSecondCirc;

	vFaces.push_back(F1);
	vFaces.push_back(F2);
}

int main()
{
	// Create a vector containing rup1 square data
	std::vector<float> Rup1_Sq;

	// Create a vector containing rup2 square data
	std::vector<float> Rup2_Sq;

    FILE* f_out = fopen("rup.obj", "wb");

	double dSquare0 = M_PI * fRadius0 * fRadius0;

	vec3 vOrigin = vec3(0, 0, 0);

    fprintf(f_out, "# 3ds Max Wavefront OBJ\r\n");

	fprintf(f_out, "#\r\n");
	fprintf(f_out, "# General model params\r\n");
	fprintf(f_out, "#\r\n");
	fprintf(f_out, "# NUM_VERTEX_IN_CIRCLE %d \r\n", NUM_VERTEX_IN_CIRCLE);
	fprintf(f_out, "# NUM_STEPS_IN_RUPOR1 %d (in fAxisCoordStep units) \r\n", NUM_STEPS_IN_RUPOR1);
	fprintf(f_out, "# fRadius0 %f (in centimeters) start rupor radius\r\n", fRadius0);
	fprintf(f_out, "# fAxisCoordStep %f (in centimeters) rupor axis coord step in centimeters\r\n", fAxisCoordStep);
	fprintf(f_out, "# fBeta %f beta exp param\r\n", fBeta);
	fprintf(f_out, "# 0.7 low frequency cutoff about %f Hz (for 34000 cm/sec sound speed)\r\n", fBeta * 34000.0f / (2 * M_PI * 1.41f));
	fprintf(f_out, "# fThickness %f (in centimeters) spacing between inner and outer surfaces\r\n", fThickness);
	fprintf(f_out, "#\r\n");
	fprintf(f_out, "\r\n");

	// Create a vector containing vec3 points coords
	std::vector<vec3> Vertices;

	// Create a vector containing vec3 points coords
	std::vector<vec3> VerticesOuter;

	// Create a vector containing Faces data
	std::vector<Face> Faces;

	// Create a vector containing Faces data
	std::vector<Face> FacesOuter;

	double fSquareCurrent;

	float fAxisCoord = 0.0f; // in centimeters

	int iStartFirstCirc = 1;
	int iStartSecondCirc = iStartFirstCirc + NUM_VERTEX_IN_CIRCLE;

	int iNumVerticesInSide = NUM_VERTEX_IN_CIRCLE * NUM_STEPS_IN_RUPOR1;

	// global to save to model text max radius
	float fRadius;

	// create first circle as base start to connect all next
	fSquareCurrent = dSquare0 * expf(fBeta * fAxisCoord);
	// save current square value of the 1st rupor
	Rup1_Sq.push_back(fSquareCurrent);

	// recalculate exponencial shaped rupor radius at current rupor-axis length coordinate
	fRadius = sqrtf(fSquareCurrent / M_PI);

	vOrigin.x = 0.0f;
	vOrigin.y = 0.0f;
	vOrigin.z = 0.0f;

	CircleGen(Vertices, vOrigin, 0, 0, 0, fRadius);
	CircleGen(VerticesOuter, vOrigin, 0, 0, 0, fRadius + fThicknessBegin);

	fAxisCoord += fAxisCoordStep; // in cm

	// rupor 1 data

	// create a set of straight circles of vertices and connect with faces
	for (int i = 1; i < NUM_STEPS_IN_RUPOR1; i++)
	{
		fSquareCurrent = dSquare0 * expf(fBeta * fAxisCoord);

		// save current square value of the 1st rupor
		Rup1_Sq.push_back(fSquareCurrent); // second and all other values

		// recalculate exponencial shaped rupor radius at current rupor-axis length coordinate
		fRadius = sqrtf(fSquareCurrent / M_PI);

		// do not spiral rotate (and not spiral axis translate of DIRECT_BEGIN and DIRECT_END number of rupor steps 
		if (i < SPECIAL_BEGIN) // Z-advance no spiral rot
		{
			vOrigin.z += fAxisCoordStep;

			CircleGen(Vertices, vOrigin, 0, 0, 0, fRadius);
			CircleGen(VerticesOuter, vOrigin, 0, 0, 0, fRadius + fThicknessBegin);

		}
		else if (i > (NUM_STEPS_IN_RUPOR1 - (SPECIAL_END + 1))) // Y-advance no spiral rot
		{
			vOrigin.z += fAxisCoordStep;

			CircleGen(Vertices, vOrigin, 0, 0, 0, fRadius);
			CircleGen(VerticesOuter, vOrigin, 0, 0, 0, fRadius + fThicknessEnd);
		}
		else // normal spiral
		{
			vOrigin.z += fAxisCoordStep;

			CircleGen(Vertices, vOrigin, 0, 0, 0, fRadius);
			CircleGen(VerticesOuter, vOrigin, 0, 0, 0, fRadius + fThickness);

		}

		CircleConnect(Faces, iStartFirstCirc, iStartSecondCirc);
		CircleConnect(FacesOuter, iStartFirstCirc + iNumVerticesInSide, iStartSecondCirc + iNumVerticesInSide);

		fAxisCoord += fAxisCoordStep; // in cm

		iStartFirstCirc += NUM_VERTEX_IN_CIRCLE;
		iStartSecondCirc += NUM_VERTEX_IN_CIRCLE;
	}

	// write vertices list
	// inner side
	for (int i = 0; i < Vertices.size(); i++)
	{
		fprintf(f_out, "v  %f %f %f\r\n", Vertices[i].x, Vertices[i].y, Vertices[i].z);
	}

	// outer side
	for (int i = 0; i < VerticesOuter.size(); i++)
	{
		fprintf(f_out, "v  %f %f %f\r\n", VerticesOuter[i].x, VerticesOuter[i].y, VerticesOuter[i].z);
	}

	fprintf(f_out, "# %d vertices\r\n", (int)(Vertices.size()+ VerticesOuter.size()));
	fprintf(f_out, "\r\n");
	fprintf(f_out, "g Rup01Inner\r\n");

	// write faces list
	for (int i = 0; i < Faces.size(); i++)
	{
		fprintf(f_out, "f  %d %d %d\r\n", Faces[i].v1, Faces[i].v2, Faces[i].v3);
	}
	fprintf(f_out, "# %d faces\r\n", (int)Faces.size());

	fprintf(f_out, "g Rup01Outer\r\n");

	// write faces list
	for (int i = 0; i < FacesOuter.size(); i++)
	{
		fprintf(f_out, "f  %d %d %d\r\n", FacesOuter[i].v1, FacesOuter[i].v2, FacesOuter[i].v3);
	}
	fprintf(f_out, "# %d faces\r\n", (int)FacesOuter.size());

	fprintf(f_out, "# Max out Diameter %f (in centimeters)\r\n", fRadius * 2.0f);
	fprintf(f_out, "# Max X length %f (in centimeters)\r\n", vOrigin.x + fRadius0 + fRadius + 2* fThickness);

    fclose(f_out);

/////////////////////////  
//     SECOND part
////////////////////////
	f_out = fopen("rup2.obj", "wb");

	// reverse squares at the steps of the first rupor
	std::reverse(Rup1_Sq.begin(), Rup1_Sq.end());

	// initial total square of the second rupor = end square of the first rupor x2 (dia ~ 1.41 of the end of first rupor)
	float fTotalSquare0_2 = Rup1_Sq[0] + dSquare0 * expf(fBeta * fAxisCoord); // end square of rup1 + start square of rup2, fAxisCoord already +1
	float fRadius0_2 = sqrtf(fTotalSquare0_2/ M_PI);

	fprintf(f_out, "# 3ds Max Wavefront OBJ\r\n");

	fprintf(f_out, "#\r\n");
	fprintf(f_out, "# General model params\r\n");
	fprintf(f_out, "#\r\n");
	fprintf(f_out, "# NUM_VERTEX_IN_CIRCLE %d \r\n", NUM_VERTEX_IN_CIRCLE);
	fprintf(f_out, "# NUM_STEPS_IN_RUPOR2 %d (in fAxisCoordStep units) \r\n", NUM_STEPS_IN_RUPOR2);
	fprintf(f_out, "# fRadius0_2 %f (in centimeters) start rupor radius\r\n", fRadius0_2);
	fprintf(f_out, "# fAxisCoordStep %f (in centimeters) rupor axis coord step in centimeters\r\n", fAxisCoordStep);
	fprintf(f_out, "# fBeta %f beta exp param\r\n", fBeta);
	fprintf(f_out, "# 0.7 low frequency cutoff about %f Hz (for 34000 cm/sec sound speed)\r\n", fBeta * 34000.0f / (2 * M_PI * 1.41f));
	fprintf(f_out, "# fThickness %f (in centimeters) spacing between inner and outer surfaces\r\n", fThickness);
	fprintf(f_out, "#\r\n");
	fprintf(f_out, "\r\n");

	// Create a vector containing vec3 points coords
	std::vector<vec3> Vertices_2;

	// Create a vector containing vec3 points coords
	std::vector<vec3> VerticesOuter_2;

	// Create a vector containing Faces data
	std::vector<Face> Faces_2;

	// Create a vector containing Faces data
	std::vector<Face> FacesOuter_2;

	// save current total square value of the 2nd rupor
	Rup2_Sq.push_back(fTotalSquare0_2);

	vOrigin.x = 0.0f;
	vOrigin.y = 0.0f;
	vOrigin.z = fAxisCoordStep * (NUM_STEPS_IN_RUPOR2 - 1); // reverse in  Z-axis

	CircleGen(Vertices_2, vOrigin, 0, 0, 0, fRadius0_2);
	CircleGen(VerticesOuter_2, vOrigin, 0, 0, 0, fRadius0_2 + fThicknessBegin);

	fAxisCoord += fAxisCoordStep; // in cm

	iStartFirstCirc = 1;
	iStartSecondCirc = iStartFirstCirc + NUM_VERTEX_IN_CIRCLE;

	iNumVerticesInSide = NUM_VERTEX_IN_CIRCLE * NUM_STEPS_IN_RUPOR2;

	// create a set of straight circles of vertices and connect with faces
	for (int i = 1; i < NUM_STEPS_IN_RUPOR2; i++)
	{
		fSquareCurrent = dSquare0 * expf(fBeta * fAxisCoord) + Rup1_Sq[i]; // sum of reversed squares of rup1 and current

		// save current square value of the 1st rupor
		Rup2_Sq.push_back(fSquareCurrent); // second and all other values

		// recalculate exponencial shaped rupor radius at current rupor-axis length coordinate
		fRadius = sqrtf(fSquareCurrent / M_PI);

		// do not spiral rotate (and not spiral axis translate of DIRECT_BEGIN and DIRECT_END number of rupor steps 
		if (i < SPECIAL_BEGIN) // Z-advance no spiral rot
		{
			vOrigin.z -= fAxisCoordStep;

			CircleGen(Vertices_2, vOrigin, 0, 0, 0, fRadius);
			CircleGen(VerticesOuter_2, vOrigin, 0, 0, 0, fRadius + fThicknessBegin);

		}
		else if (i > (NUM_STEPS_IN_RUPOR2 - (SPECIAL_END + 1))) // Y-advance no spiral rot
		{
			vOrigin.z -= fAxisCoordStep;

			CircleGen(Vertices_2, vOrigin, 0, 0, 0, fRadius);
			CircleGen(VerticesOuter_2, vOrigin, 0, 0, 0, fRadius + fThicknessEnd);
		}
		else // normal spiral
		{
			vOrigin.z -= fAxisCoordStep;

			CircleGen(Vertices_2, vOrigin, 0, 0, 0, fRadius);
			CircleGen(VerticesOuter_2, vOrigin, 0, 0, 0, fRadius + fThickness);

		}

		CircleConnect(Faces_2, iStartFirstCirc, iStartSecondCirc);
		CircleConnect(FacesOuter_2, iStartFirstCirc + iNumVerticesInSide, iStartSecondCirc + iNumVerticesInSide);

		fAxisCoord += fAxisCoordStep; // in cm

		iStartFirstCirc += NUM_VERTEX_IN_CIRCLE;
		iStartSecondCirc += NUM_VERTEX_IN_CIRCLE;
	}

	// write vertices list
	// inner side
	for (int i = 0; i < Vertices_2.size(); i++)
	{
		fprintf(f_out, "v  %f %f %f\r\n", Vertices_2[i].x, Vertices_2[i].y, Vertices_2[i].z);
	}

	// outer side
	for (int i = 0; i < VerticesOuter_2.size(); i++)
	{
		fprintf(f_out, "v  %f %f %f\r\n", VerticesOuter_2[i].x, VerticesOuter_2[i].y, VerticesOuter_2[i].z);
	}

	fprintf(f_out, "# %d vertices\r\n", (int)(Vertices_2.size() + VerticesOuter_2.size()));
	fprintf(f_out, "\r\n");
	fprintf(f_out, "g Rup02Inner\r\n");

	// write faces list
	for (int i = 0; i < Faces_2.size(); i++)
	{
		fprintf(f_out, "f  %d %d %d\r\n", Faces_2[i].v1, Faces_2[i].v2, Faces_2[i].v3);
	}
	fprintf(f_out, "# %d faces\r\n", (int)Faces_2.size());

	fprintf(f_out, "g Rup02Outer\r\n");

	// write faces list
	for (int i = 0; i < FacesOuter_2.size(); i++)
	{
		fprintf(f_out, "f  %d %d %d\r\n", FacesOuter_2[i].v1, FacesOuter_2[i].v2, FacesOuter_2[i].v3);
	}
	fprintf(f_out, "# %d faces\r\n", (int)FacesOuter_2.size());

	fprintf(f_out, "# Max out Diameter %f (in centimeters)\r\n", fRadius * 2.0f);
	fprintf(f_out, "# Max X length %f (in centimeters)\r\n", vOrigin.x + fRadius0 + fRadius + 2 * fThickness);

	fclose(f_out);

/////////////////////////  
//      THIRD part
/////////////////////////
	f_out = fopen("rup3.obj", "wb");

	// reverse squares at the steps of the first rupor
	std::reverse(Rup2_Sq.begin(), Rup2_Sq.end());

	// initial total square of the second rupor = end square of the first rupor x2 (dia ~ 1.41 of the end of first rupor)
	double dTotalSquare0_3 = Rup2_Sq[0] + dSquare0 * expf(fBeta * fAxisCoord); // end square of rup1 + start square of rup2, fAxisCoord already +1
	float fRadius0_3 = sqrtf(dTotalSquare0_3 / M_PI);

	fprintf(f_out, "# 3ds Max Wavefront OBJ\r\n");

	fprintf(f_out, "#\r\n");
	fprintf(f_out, "# General model params\r\n");
	fprintf(f_out, "#\r\n");
	fprintf(f_out, "# NUM_VERTEX_IN_CIRCLE %d \r\n", NUM_VERTEX_IN_CIRCLE);
	fprintf(f_out, "# NUM_STEPS_IN_RUPOR3 %d (in fAxisCoordStep units) \r\n", NUM_STEPS_IN_RUPOR3);
	fprintf(f_out, "# fRadius0_3 %f (in centimeters) start rupor radius\r\n", fRadius0_3);
	fprintf(f_out, "# fAxisCoordStep %f (in centimeters) rupor axis coord step in centimeters\r\n", fAxisCoordStep);
	fprintf(f_out, "# fBeta %f beta exp param\r\n", fBeta);
	fprintf(f_out, "# 0.7 low frequency cutoff about %f Hz (for 34000 cm/sec sound speed)\r\n", fBeta * 34000.0f / (2 * M_PI * 1.41f));
	fprintf(f_out, "# fThickness %f (in centimeters) spacing between inner and outer surfaces\r\n", fThickness);
	fprintf(f_out, "#\r\n");
	fprintf(f_out, "\r\n");

	// Create a vector containing vec3 points coords
	std::vector<vec3> Vertices_3;

	// Create a vector containing vec3 points coords
	std::vector<vec3> VerticesOuter_3;

	// Create a vector containing Faces data
	std::vector<Face> Faces_3;

	// Create a vector containing Faces data
	std::vector<Face> FacesOuter_3;

	vOrigin.x = 0.0f;
	vOrigin.y = 0.0f;
	vOrigin.z = 0.0f; // no reverse in  Z-axis

	CircleGen(Vertices_3, vOrigin, 0, 0, 0, fRadius0_3);
	CircleGen(VerticesOuter_3, vOrigin, 0, 0, 0, fRadius0_3 + fThicknessBegin);

	fAxisCoord += fAxisCoordStep; // in cm

	iStartFirstCirc = 1;
	iStartSecondCirc = iStartFirstCirc + NUM_VERTEX_IN_CIRCLE;

	iNumVerticesInSide = NUM_VERTEX_IN_CIRCLE * NUM_STEPS_IN_RUPOR3;

	// create a set of straight circles of vertices and connect with faces
	for (int i = 1; i < NUM_STEPS_IN_RUPOR3; i++)
	{

		fSquareCurrent = dSquare0 * expf(fBeta * fAxisCoord) + Rup2_Sq[i]; // sum of reversed squares of rup2 and current

		// recalculate exponencial shaped rupor radius at current rupor-axis length coordinate
		fRadius = sqrtf(fSquareCurrent / M_PI);

		// do not spiral rotate (and not spiral axis translate of DIRECT_BEGIN and DIRECT_END number of rupor steps 
		if (i < SPECIAL_BEGIN) // Z-advance no spiral rot
		{
			vOrigin.z += fAxisCoordStep;

			CircleGen(Vertices_3, vOrigin, 0, 0, 0, fRadius);
			CircleGen(VerticesOuter_3, vOrigin, 0, 0, 0, fRadius + fThicknessBegin);

		}
		else if (i > (NUM_STEPS_IN_RUPOR3 - (SPECIAL_END + 1))) // Y-advance no spiral rot
		{
			vOrigin.z += fAxisCoordStep;

			CircleGen(Vertices_3, vOrigin, 0, 0, 0, fRadius);
			CircleGen(VerticesOuter_3, vOrigin, 0, 0, 0, fRadius + fThicknessEnd);
		}
		else // normal spiral
		{
			vOrigin.z += fAxisCoordStep;

			CircleGen(Vertices_3, vOrigin, 0, 0, 0, fRadius);
			CircleGen(VerticesOuter_3, vOrigin, 0, 0, 0, fRadius + fThickness);

		}

		CircleConnect(Faces_3, iStartFirstCirc, iStartSecondCirc);
		CircleConnect(FacesOuter_3, iStartFirstCirc + iNumVerticesInSide, iStartSecondCirc + iNumVerticesInSide);

		fAxisCoord += fAxisCoordStep; // in cm

		iStartFirstCirc += NUM_VERTEX_IN_CIRCLE;
		iStartSecondCirc += NUM_VERTEX_IN_CIRCLE;
	}

	// write vertices list
	// inner side
	for (int i = 0; i < Vertices_3.size(); i++)
	{
		fprintf(f_out, "v  %f %f %f\r\n", Vertices_3[i].x, Vertices_3[i].y, Vertices_3[i].z);
	}

	// outer side
	for (int i = 0; i < VerticesOuter_3.size(); i++)
	{
		fprintf(f_out, "v  %f %f %f\r\n", VerticesOuter_3[i].x, VerticesOuter_3[i].y, VerticesOuter_3[i].z);
	}

	fprintf(f_out, "# %d vertices\r\n", (int)(Vertices_3.size() + VerticesOuter_3.size()));
	fprintf(f_out, "\r\n");
	fprintf(f_out, "g Rup03Inner\r\n");

	// write faces list
	for (int i = 0; i < Faces_3.size(); i++)
	{
		fprintf(f_out, "f  %d %d %d\r\n", Faces_3[i].v1, Faces_3[i].v2, Faces_3[i].v3);
	}
	fprintf(f_out, "# %d faces\r\n", (int)Faces_3.size());

	fprintf(f_out, "g Rup03Outer\r\n");

	// write faces list
	for (int i = 0; i < FacesOuter_3.size(); i++)
	{
		fprintf(f_out, "f  %d %d %d\r\n", FacesOuter_3[i].v1, FacesOuter_3[i].v2, FacesOuter_3[i].v3);
	}
	fprintf(f_out, "# %d faces\r\n", (int)FacesOuter_3.size());

	fprintf(f_out, "# Max out Diameter %f (in centimeters)\r\n", fRadius * 2.0f);
	fprintf(f_out, "# Max X length %f (in centimeters)\r\n", vOrigin.x + fRadius0 + fRadius + 2 * fThickness);

	fclose(f_out);

}
