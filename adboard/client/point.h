#ifndef POINT_H
#define POINT_H


//像素点结构体
typedef struct Point
{
	int x;
	int y;
}PIXELPOINT;

//坐标结构体
typedef struct Coordinate
{
	int x;
	int y;
}COORDINATEPOINT;

//lcd矩形
typedef struct Rect
{
	PIXELPOINT pointA;
	PIXELPOINT pointB;
}RECT;

#endif