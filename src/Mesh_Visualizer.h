#include <vector>
#include <opencv2/opencv.hpp>
#include <open3d/Open3D.h>
#include <Eigen/Core>

class Mesh_Visualizer {
    public:
        Mesh_Visualizer(int width, int height, std::vector<Eigen::Vector3d> &vertices, std::vector<Eigen::Vector3i> &triangles, Eigen::Matrix3d K, std::shared_ptr<open3d::geometry::TriangleMesh> mesh);
        ~Mesh_Visualizer();
        void initImageParams(cv::Mat &frame);
        void UpdateMesh(cv::Mat &frame, std::shared_ptr<open3d::geometry::TriangleMesh> &mesh);
        
    private:
        void updateFrame(cv::Mat &frame);
        void updateErrorMap(cv::Mat &frame); 
        open3d::visualization::ViewControl view_control;
        std::shared_ptr<open3d::geometry::TriangleMesh> mesh_ = nullptr;
        open3d::visualization::Visualizer visualizer;
        open3d::geometry::Image texture_image, texture_error_map;

        std::vector<Eigen::Vector3d> vertices_;
        std::vector<Eigen::Vector3i> triangles_;
        std::vector<Eigen::Vector2d> uv_coordinates_;

        Eigen::Matrix3d K_;

        int width_ = 0;
        int height_ = 0;
        int num_channels_ = 0;
        int bytes_per_channel_ = sizeof(uint8_t);

};