/*Copyright (c) 2025 Tristan Wellman
 *
 * This is for parsing .obj files as well as OpenFOAM mesh vert/face files
 *
 * */
#ifndef MESHPARSE_H
#define MESHPARSE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "util.h"

#define MAXDATA 10000

#define VSIZE 3 /*x,y,z*/
#define TEXSIZE 2 /*u,v*/
#define NORMSIZE 3 /*dx,dy,dz*/
#define ISIZE 4 /*v1,v2,v3,v4*/

typedef struct {
	uint16_t **data;
	int cap;
	int size;
	int total;
} DynArrI;

typedef struct {
	float **data;
	int cap;
	int size;
	int total;
} DynArrD;

typedef struct {
	DynArrD verts;
	DynArrD vertTex;
	DynArrD vertNorms;
	DynArrI indices;
	DynArrI normInds;
	DynArrI texInds;
	FILE *f;
	char *label;
} OEMesh;

typedef struct {
	DynArrD verts;
	/*The faces need to be converted to indices!*/
	DynArrI indices;
	DynArrD faces;
} OEFOAMMesh;

/*This sketchy void ptr expects a FILE ptr*/
typedef struct {
	void *file;
	void *meshptr;
} OEThreadArg;

void OEParseFOAMObj(char* path, OEFOAMMesh* mesh);
void OEParseObj(char *file, OEMesh *mesh);
void scaleMesh(OEMesh *mesh, float s); 


#ifdef __cplusplus
}
#endif
#endif
