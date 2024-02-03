#pragma once
/*
*  光栅化三角形类
*/
#ifndef RST_TRI_H
#define RST_TRI_H
#include "geometry.h"
class Rst_Tri
{
	public :
		vec4 vertex[3];
		vec3 normal[3];
		vec2 texcoords[3];
		Rst_Tri()
		{
			vertex[0] = { 0, 0, 0 ,1 };
			vertex[1] = { 0, 0, 0 ,1};
			vertex[2] = { 0, 0, 0 ,1 };

			normal[0] = { 0, 0, 0 };
			normal[1] = { 0, 0, 0 };
			normal[2] = { 0, 0, 0 };

			texcoords[0] = { 0,0 };
			texcoords[1] = { 0,0 };
			texcoords[2] = { 0,0 };
		}
		void set_vertex(Point3 a,Point3 b,Point3 c)
		{
			this->vertex[0][0] = a.x();
			this->vertex[0][1] = a.y();
			this->vertex[0][2] = a.z();

			this->vertex[1][0] = b.x();
			this->vertex[1][1] = b.y();
			this->vertex[1][2] = b.z();

			this->vertex[2][0] = c.x();
			this->vertex[2][1] = c.y();
			this->vertex[2][2] = c.z();

		}
		void set_normal(Vec3 a, Vec3 b, Vec3 c)
		{
			this->normal[0].x = a.x();
			this->normal[0].y = a.y();
			this->normal[0].z = a.z();

			this->normal[1].x = b.x();
			this->normal[1].y = b.y();
			this->normal[1].z = b.z();

			this->normal[2].x = c.x();
			this->normal[2].y = c.y();
			this->normal[2].z = c.z();
		}
		void set_texcoord(std::pair<double, double> a, std::pair<double, double> b, std::pair<double, double> c)
		{
			this->texcoords[0].x = a.first;
			this->texcoords[0].y = a.second;

			this->texcoords[1].x = b.first;
			this->texcoords[1].y = b.second;

			this->texcoords[2].x = c.first;
			this->texcoords[2].y = c.second;
		}
		void vertex_homo_divi()
		{
			this->vertex[0][0] /= this->vertex[0][3];
			this->vertex[0][1] /= this->vertex[0][3];
			this->vertex[0][2] /= this->vertex[0][3];
			this->vertex[0][3] /= this->vertex[0][3];

			this->vertex[1][0] /= this->vertex[1][3];
			this->vertex[1][1] /= this->vertex[1][3];
			this->vertex[1][2] /= this->vertex[1][3];
			this->vertex[1][3] /= this->vertex[1][3];

			this->vertex[2][0] /= this->vertex[2][3];
			this->vertex[2][1] /= this->vertex[2][3];
			this->vertex[2][2] /= this->vertex[2][3];
			this->vertex[2][3] /= this->vertex[2][3];
		}

};
#endif // !RST_TRI_H