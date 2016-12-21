#ifndef LIDAR_ALIGN_SENSORS_H_
#define LIDAR_ALIGN_SENSORS_H_

#include <random>

#include <pcl/common/transforms.h>
#include <pcl/io/ply_io.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl/filters/statistical_outlier_removal.h>

#include <kindr/minimal/quat-transformation.h>

typedef std::string LidarId;
typedef float Scalar;
// this must be at least 64 bit and signed or things will break
typedef long long int Timestamp;

typedef kindr::minimal::QuatTransformationTemplate<Scalar> Transform;
typedef kindr::minimal::AngleAxisTemplate<Scalar> AngleAxis;
typedef pcl::PointXYZI Point;
typedef pcl::PointCloud<Point> Pointcloud;

class OdomTformData {
 public:
  OdomTformData(Timestamp timestamp_us, Transform T_o0_ot);

  const Transform& getTransform() const;
  const Timestamp& getTimestamp() const;

 private:
  Transform T_o0_ot_;
  Timestamp timestamp_us_;
};

class Odom {
 public:
  void addRawOdomData(const Timestamp& timestamp_us,
                      const Scalar& linear_velocity,
                      const Scalar& angular_velocity);

  Transform getOdomTransform(const Timestamp timestamp_us,
                             const size_t start_idx = 0,
                             size_t* match_idx = nullptr) const;

 private:
  std::vector<OdomTformData> data_;
};

class Scan {
 public:
  Scan(const Pointcloud& pointcloud);

  void setOdomTransform(const Odom& odom, const size_t start_idx,
                        size_t* match_idx);

  const Transform& getOdomTransform() const;

  const Pointcloud& getRawPointcloud() const;

  void getTimeAlignedPointcloud(const Transform& T_o_l, Pointcloud* pointcloud) const;

 private:
  Timestamp timestamp_us_;  // signed to allow simpler comparisons
  Pointcloud raw_points_;
  std::vector<Transform>
      T_o0_ot_;  // absolute odom transform to each point in pointcloud

  bool odom_transform_set_;
};

class Lidar {
 public:
  Lidar(const LidarId& lidar_id, const Scalar& min_point_dist,
        const Scalar& max_point_dist);

  const size_t getNumberOfScans() const;

  //note points are appended so any points in *pointcloud are preserved
  void getCombinedPointcloud(Pointcloud* pointcloud) const;

  const LidarId& getId() const;

  void addPointcloud(const Pointcloud& pointcloud);

  void setOdomOdomTransforms(const Odom& odom);

  void setOdomLidarTransform(const Transform& T_o_l);

  // used for debugging frames
  void saveCombinedPointcloud(const std::string& file_path) const;

  const Transform& getOdomLidarTransform() const;

 private:
  LidarId lidar_id_;
  Transform T_o_l_;  // transform from lidar to odometry

  std::vector<Scan> scans_;

  Scalar max_point_dist_;
  Scalar min_point_dist_;

  void filterPointcloud(const Pointcloud& in, Pointcloud* out);
};

class LidarArray {
 public:

  const size_t getNumberOfLidars() const;

  void addPointcloud(const LidarId& lidar_id, const Pointcloud& pointcloud);

  void getCombinedPointcloud(Pointcloud* pointcloud) const;

  const Lidar& getLidar(const LidarId& lidar_id) const;

  std::vector<Lidar>& getLidarVector();

  const std::vector<Lidar>& getLidarVector() const;

  bool hasAtleastNScans(const size_t n) const;

  void setOdomOdomTransforms(const Odom& odom);

 private:
  // while it is nice to refer to lidars by id, we often just need a vector of
  // them (thus why the lidars are not in the map directly)
  std::map<LidarId, size_t> id_to_idx_map_;
  std::vector<Lidar> lidar_vector_;
};

#endif