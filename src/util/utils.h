#ifndef UTILS_H
#define UTILS_H

#include <opencv2/opencv.hpp>
#include <open3d/Open3D.h>
#include <Eigen/Core>
#include <Eigen/Geometry>

#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace utils {
    void getMeshColon(const YAML::Node &config, std::string path, std::vector<Eigen::Vector3d> &vertices, std::vector<Eigen::Vector3i> &triangles) {
        std::shared_ptr<open3d::geometry::TriangleMesh> mesh;
        // mesh = createRefMesh(config, gt_id);
        mesh = open3d::io::CreateMeshFromFile(path);
        mesh = mesh->SimplifyQuadricDecimation(config["GT_Mesh"]["simplification"].as<int>(), std::numeric_limits<double>::infinity(), 1.0);

        for(int i=0; i<mesh->vertices_.size();i++)
            vertices.push_back(mesh->vertices_[i]);

        for(int i=0; i<mesh->triangles_.size();i++)
            triangles.push_back(mesh->triangles_[i]);
            
        // open3d::visualization::Visualizer vis;
        // vis.CreateVisualizerWindow("adsad");
        // vis.AddGeometry(mesh);
        // vis.Run();
        // vis.DestroyVisualizerWindow();
    }

    void getMesh(std::string path, std::vector<Eigen::Vector3d> &vertices, std::vector<Eigen::Vector3i> &triangles) {
        std::shared_ptr<open3d::geometry::TriangleMesh> mesh;
        // mesh = createRefMesh(config, gt_id);
        mesh = open3d::io::CreateMeshFromFile(path);

        for(int i=0; i<mesh->vertices_.size();i++)
            vertices.push_back(mesh->vertices_[i]);

        for(int i=0; i<mesh->triangles_.size();i++)
            triangles.push_back(mesh->triangles_[i]);
            
        // open3d::visualization::Visualizer vis;
        // vis.CreateVisualizerWindow("adsad");
        // vis.AddGeometry(mesh);
        // vis.Run();
        // vis.DestroyVisualizerWindow();
    }

    // this got used to create a reference mesh for
    void createRefMeshDepthImages(const YAML::Node &config, std::string gt_id, std::vector<Eigen::Vector3d> &vertices, std::vector<Eigen::Vector3i> &triangles) {
        std::string file = config["System"]["reference_file_path"].as<std::string>() + gt_id + ".exr";
        Eigen::Matrix3d K,K_inv;
        K <<    config["Image"]["fx"].as<double>(), 0.000000, config["Image"]["cx"].as<double>(),
                0.000000, config["Image"]["fy"].as<double>(), config["Image"]["cy"].as<double>(),
                0.000000, 0.000000, 1.000000;
        K_inv = K.inverse();
        double fx = config["Image"]["fx"].as<double>() * config["Image"]["cx"].as<double>() / 10.26;
        double fy = config["Image"]["fy"].as<double>()* config["Image"]["cy"].as<double>() / 10.26;
        double cx = config["Image"]["cx"].as<double>()/2;
        double cy = config["Image"]["cy"].as<double>()/2;
        
        cv::Mat frame = cv::imread(file, cv::IMREAD_ANYDEPTH);
        int height = frame.rows;
        int width = frame.cols;
        std::vector<Eigen::Vector3d> pts;
        // std::cout << frame.size() << std::endl; exit(1);
        // Iteriere über alle Bildpunkte
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                // Pixelwert am aktuellen Punkt abrufen (für ein Graustufenbild)
                float d = frame.at<float>(y, x);
                double depth = (double)d;
                if (depth == 0)
                    continue;
                depth *= 5;
                // std::cout << depth << std::endl;
                Eigen::Vector3d tmp, uvt;
                uvt << x,y,1;
                // tmp = K_inv*uvt*(depth);
                tmp <<  (x-cx)*depth/fx, 
                        (y-cy)*depth/fy, 
                        depth; 
                pts.push_back(tmp);
                // std::cout <<  fx << " " << (x-cx)*depth/fx << " " << (y-cy)*depth/fy << " " << depth << std::endl;
                // Hier kannst du mit dem Pixelwert arbeiten (z.B. bearbeiten, anzeigen usw.)
                // Zum Beispiel: std::cout << "Pixelwert an (" << x << ", " << y << "): " << (int)pixel_value << std::endl;
            }
        }
        
        open3d::geometry::PointCloud pcd;
        pcd.points_.resize(pts.size());
        pcd.points_ = pts;
        
        // std::cout << pcd.points_.size() << std::endl; exit(1);
        std::shared_ptr<open3d::geometry::PointCloud> pc;
        pc = std::make_shared<open3d::geometry::PointCloud>(pcd);


        pc->EstimateNormals();
        pc->OrientNormalsConsistentTangentPlane(config["GT_Mesh"]["OrientNormalsConsistentTangentPlane"].as<int>());
        double depth1 = config["GT_Mesh"]["Poisson_depth"].as<double>();
        double width1 = config["GT_Mesh"]["Poisson_width"].as<double>();
        std::tuple<std::shared_ptr<open3d::geometry::TriangleMesh>, std::vector<double>> meshDensity = open3d::geometry::TriangleMesh::CreateFromPointCloudPoisson(*pc, depth1, width1);

        std::shared_ptr< open3d::geometry::TriangleMesh > mesh = std::get<0>(meshDensity);
        std::vector<double> density = std::get<1>(meshDensity);
        std::vector<size_t> filtered;
        for (int j=0; j<density.size(); j++) {
            auto vertex = mesh->vertices_[j];
            double u = fx * vertex(0) / vertex(2) + cx;
            double v = fy * vertex(1) / vertex(2) + cy;
            if( (density[j] < config["GT_Mesh"]["threshold"].as<double>())|| (u < 0 || u > 512 || v < 0 || v > 512))
                filtered.push_back(j);
        }
        
    

        mesh->RemoveVerticesByIndex(filtered);
        open3d::io::WriteTriangleMesh("mesh.obj",*mesh);
        open3d::io::WritePointCloud("pc.ply",*pc);
        // mesh->ComputeTriangleNormals();

        // mesh = mesh->SimplifyQuadricDecimation(config["GT_Mesh"]["simplification"].as<int>(), std::numeric_limits<double>::infinity(), 1.0);
        
        
        // open3d::visualization::DrawGeometries(*pcds);
        open3d::visualization::Visualizer vis;
        vis.CreateVisualizerWindow("adsad");
        vis.AddGeometry(mesh);
        vis.Run();
        vis.DestroyVisualizerWindow();
        // exit(1);
        // open3d::visualization::DrawGeometries({pcd});
        // cv::imshow("asd", cv::imread(file));
        // cv::waitKey(0);
    }


    void createRefMesh(const YAML::Node &config, uint gt_id, std::vector<Eigen::Vector3d> &vertices, std::vector<Eigen::Vector3i> &triangles) {
    // create reference mesh
    std::string file = config["System"]["reference_file_path"].as<std::string>() + std::to_string(gt_id) + ".txt";
    Eigen::Matrix3d K;
    K <<    config["Image"]["fx"].as<double>(), 0.000000, config["Image"]["cx"].as<double>(),
            0.000000, config["Image"]["fy"].as<double>(), config["Image"]["cy"].as<double>(),
            0.000000, 0.000000, 1.000000;

    auto pc = open3d::io::CreatePointCloudFromFile(file, "xyz");

    auto& points = pc->points_;
    std::vector<Eigen::Vector3d> valid_points;
    for (const auto& point : points) {
        if (point.x() != 0 || point.y() != 0 || point.z() != 0) {
            valid_points.push_back(point);
        }
    }
    pc->points_ = valid_points;
    for (size_t i = 0; i < pc->points_.size(); ++i) {
        auto vertex = pc->points_[i];
        double u = K(0, 0) * vertex(0) / vertex(2) + K(0, 2);
        double v = K(1, 1) * vertex(1) / vertex(2) + K(1, 2);
        
        if (u < 0 || u > 360 || v < 0 || v > 288) {

        }
    }

    pc->EstimateNormals();
    pc->OrientNormalsConsistentTangentPlane(config["GT_Mesh"]["OrientNormalsConsistentTangentPlane"].as<int>());
    double depth = config["GT_Mesh"]["Poisson_depth"].as<double>();
    double width = config["GT_Mesh"]["Poisson_width"].as<double>();
    std::tuple<std::shared_ptr<open3d::geometry::TriangleMesh>, std::vector<double>> meshDensity = open3d::geometry::TriangleMesh::CreateFromPointCloudPoisson(*pc, depth, width);

    std::shared_ptr< open3d::geometry::TriangleMesh > mesh = std::get<0>(meshDensity);
    mesh->ComputeTriangleNormals();

    std::vector<size_t> bad_vertices;
    for (size_t i = 0; i < mesh->vertices_.size(); ++i) {
        auto vertex = mesh->vertices_[i];
        double u = K(0, 0) * vertex(0) / vertex(2) + K(0, 2);
        double v = K(1, 1) * vertex(1) / vertex(2) + K(1, 2);
        
        if (u < 0 || u > config["Image"]["width"].as<int>() || v < 0 || v > config["Image"]["height"].as<int>()) {
            bad_vertices.push_back(i);
        }
        // else {
        //     good_vertices.push_back(1);
        // }
    }

    mesh->RemoveVerticesByIndex(bad_vertices);

    mesh = mesh->SimplifyQuadricDecimation(config["GT_Mesh"]["simplification"].as<int>(), std::numeric_limits<double>::infinity(), 1.0);
    
    for(int i=0; i<mesh->vertices_.size();i++)
            vertices.push_back(mesh->vertices_[i]);

        for(int i=0; i<mesh->triangles_.size();i++)
            triangles.push_back(mesh->triangles_[i]);
    
    
    // open3d::io::WriteTriangleMesh("output_mesh.ply", *mesh);
    // exit(1);
    }
    
}

#endif 