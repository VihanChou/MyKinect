// pcdCreate.cpp: 定义控制台应用程序的入口点。
//从depth.png rgb.png 生成点云文件

#include "stdafx.h"
#include <iostream>// C++ 标准库
#include <string>
#include <opencv2/core/core.hpp>// OpenCV 库
#include <opencv2/highgui/highgui.hpp>
#include <pcl/io/pcd_io.h>   // PCL 库
#include <pcl/point_types.h>
using namespace std;
using namespace cv;

// 定义点云类型
typedef pcl::PointXYZ PointT;
typedef pcl::PointCloud<PointT>  PointCloud;

// 相机内参
const double camera_factor = 1000;
const double camera_cx = 325.5;
const double camera_cy = 253.5;
const double camera_fx = 518.0;
const double camera_fy = 519.0;

// 主函数 
int main(int argc , char** argv)
{
	// 图像矩阵
	Mat  depth;
	depth = cv::imread("C:/Users/1234/Desktop/i_depth.png" , -1);

	// 使用智能指针，创建一个空点云。这种指针用完会自动释放。
	PointCloud::Ptr cloud(new PointCloud);

	// 遍历深度图
	for (int m = 0; m<depth.rows; m++)      //行
	{
		for (int n = 0; n<depth.cols; n++)  //列
		{
			// 获取深度图中(m,n)处的值
			ushort d = depth.ptr<ushort>(m)[n];

			// d 可能没有值，若如此，跳过此点
			if (d==0)
				continue;

			// d 存在值，则向点云增加一个点
			PointT p;

			// 计算这个点的空间坐标
			p.z = double(d)/camera_factor;
			p.x = (n-camera_cx) * p.z/camera_fx;
			p.y = (m-camera_cy) * p.z/camera_fy;

			// 把p加入到点云中
			cloud->points.push_back(p);
		}
	}

	// 设置并保存点云
	cloud->height = 1;
	cloud->width = cloud->points.size( );
	cloud->is_dense = false; //如果没有无效点，则为真
	if (pcl::io::savePCDFile("C:/Users/1234/Desktop/testvihan.pcd" , *cloud))
	{
		cout<<"Pcd文件已生成"<<endl;
	}
	else
	{
		cout<<"Pcd文件生成失败"<<endl;
	}

	cout<<"point cloud size = "<<cloud->points.size( )<<endl;
	// 清除数据并退出
	cloud->points.clear( );
	cout<<"Point cloud saved."<<endl;

	return 0;
}