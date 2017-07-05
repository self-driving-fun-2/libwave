#ifndef WAVE_LASERODOM_HPP
#define WAVE_LASERODOM_HPP

#include <vector>
#include <array>
#include <algorithm>
#include <utility>
#include <chrono>
#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include "wave/odometry/kdtreetype.hpp"
#include "wave/odometry/PointXYZIR.hpp"
#include "wave/odometry/PointXYZIT.hpp"
#include "wave/odometry/laser_odom_residuals.hpp"
#include "wave/containers/measurement_container.hpp"
#include "wave/containers/measurement.hpp"
#include "wave/utils/math.hpp"

namespace wave {

using IMUMeasurement = Measurement<Vec6, char>;
using unlong = unsigned long;
using PointType = pcl::PointXYZI;

struct LaserOdomParams {
    int opt_iters = 25;
    float scan_period = 0.1;     // Seconds
    float occlusion_tol = 0.1;   // Don't know units
    float parallel_tol = 0.002;  // ditto
    float keypt_radius = 0.05;   // m2
    float edge_tol = 0.1;  // Edge features must have score higher than this
    float flat_tol = 0.1;  // Plane features must have score lower than this
    int max_ticks = 3600;  // encoder ticks per revolution
    int n_edge = 40;       // How many edge features to pick out
    int n_flat = 100;      // How many plane features to pick out
    unlong knn = 5;        // 1/2 nearest neighbours for computing curvature
    unlong n_ring = 32;    // number of laser-detector pairs
};

class LaserOdom {
 public:
    LaserOdom(const LaserOdomParams params);
    void addPoints(const std::vector<PointXYZIR> &pts,
                   const int tick,
                   TimeType stamp);
    void addIMU(std::vector<double> linacc, Quaternion orientation);
    pcl::PointCloud<PointXYZIT> edges, flats;
    // transform is stored as an axis-angle rotation [012] and a
    // displacement [345]
    std::array<double, 6> cur_transform;

 private:
    LaserOdomParams param;
    bool initialized = false;

    void transformToStart();

    void undistort();
    void rollover(TimeType stamp);
    void resetIMU(TimeType stamp);
    void computeCurvature();
    void prefilter();
    void generateFeatures();
    void buildTrees();
    void match();

    PointXYZIT applyIMU(const PointXYZIT &pt);

    // store for the IMU integral
    MeasurementContainer<IMUMeasurement> imu_trans;

    std::vector<double> lin_vel = {0, 0, 0};
    TimeType prv_time, cur_time;
    static float l2sqrd(const PointXYZIT &p1, const PointXYZIT &p2);
    static float l2sqrd(const PointXYZIT &pt);
    static PointXYZIT scale(const PointXYZIT &pt, const float scale);
    void flagNearbyPoints(const unlong ring, const unlong index);
    std::vector<std::vector<std::pair<bool, float>>> cur_curve;
    std::vector<std::vector<std::pair<unlong, float>>> filter;
    std::vector<pcl::PointCloud<PointXYZIT>> cur_scan;
    FeatureKDTree<double> prv_edges, prv_flats;
    kd_tree_t* edge_idx;
    kd_tree_t* flat_idx;
};

}  // namespace wave

#endif  // WAVE_LASERODOM_HPP