#include <ros/ros.h>

#include <tf/tf.h>
#include <tf/transform_listener.h>
#include <tf/transform_broadcaster.h>

#include <interactive_markers/interactive_marker_server.h>
#include <interactive_marker_helpers/interactive_marker_helpers.h>

#include <interactive_markers/menu_handler.h>
#include <jsk_interactive_marker/SetPose.h>
#include <jsk_interactive_marker/MarkerSetPose.h>

#include <math.h>
#include <jsk_interactive_marker/MarkerMenu.h>
#include <jsk_interactive_marker/MarkerPose.h>
#include <jsk_interactive_marker/MoveObject.h>

#include <std_msgs/Int8.h>

#include "urdf_parser/urdf_parser.h"
#include <iostream>
#include <fstream>
using namespace urdf;
using namespace std;


class UrdfModelMarker {
 public:
  //  UrdfModelMarker(string file);
  UrdfModelMarker(string file, boost::shared_ptr<interactive_markers::InteractiveMarkerServer> server);
  UrdfModelMarker(string model_name, string model_file, string frame_id, geometry_msgs::Pose root_pose, boost::shared_ptr<interactive_markers::InteractiveMarkerServer> server);
  UrdfModelMarker();

  void addMoveMarkerControl(visualization_msgs::InteractiveMarker &int_marker, boost::shared_ptr<const Link> link, bool root);
  void addGraspPointControl(visualization_msgs::InteractiveMarker &int_marker, std::string link_frame_name_);

  void publishMarkerPose ( const visualization_msgs::InteractiveMarkerFeedbackConstPtr &feedback);
  void publishMarkerMenu( const visualization_msgs::InteractiveMarkerFeedbackConstPtr &feedback, int menu );
  void publishMoveObject( const visualization_msgs::InteractiveMarkerFeedbackConstPtr &feedback );
  void proc_feedback( const visualization_msgs::InteractiveMarkerFeedbackConstPtr &feedback, string parent_frame_id, string frame_id);

  void graspPoint_feedback( const visualization_msgs::InteractiveMarkerFeedbackConstPtr &feedback, string link_name);
  void moveCB( const visualization_msgs::InteractiveMarkerFeedbackConstPtr &feedback );
  void setPoseCB( const visualization_msgs::InteractiveMarkerFeedbackConstPtr &feedback );



  visualization_msgs::InteractiveMarkerControl makeMeshMarkerControl(const std::string &mesh_resource, const geometry_msgs::PoseStamped &stamped, float scale, const std_msgs::ColorRGBA &color, bool use_color);
  visualization_msgs::InteractiveMarkerControl makeMeshMarkerControl(const std::string &mesh_resource, const geometry_msgs::PoseStamped &stamped, float scale);
  visualization_msgs::InteractiveMarkerControl makeMeshMarkerControl(const std::string &mesh_resource, const geometry_msgs::PoseStamped &stamped, float scale, const std_msgs::ColorRGBA &color);

  void setOriginalPose(boost::shared_ptr<const Link> link);
  void addChildLinkNames(boost::shared_ptr<const Link> link, bool root, bool init);

  geometry_msgs::Transform Pose2Transform(geometry_msgs::Pose pose_msg);
  geometry_msgs::Pose UrdfPose2Pose( const urdf::Pose pose);
  void CallSetDynamicTf(string parent_frame_id, string frame_id, geometry_msgs::Transform transform);

  int main(string file);

  void graspPointCB( const visualization_msgs::InteractiveMarkerFeedbackConstPtr &feedback );
 private:
  ros::NodeHandle nh_;
  ros::NodeHandle pnh_;
  boost::shared_ptr<interactive_markers::InteractiveMarkerServer> server_;
  ros::Publisher pub_;
  ros::Publisher pub_move_;
  ros::Publisher pub_move_object_;
  ros::ServiceServer serv_reset_;
  ros::ServiceServer serv_set_;
  ros::ServiceServer serv_markers_set_;
  ros::ServiceServer serv_markers_del_;

  interactive_markers::MenuHandler model_menu_;

  tf::TransformListener tfl_;
  tf::TransformBroadcaster tfb_;

  ros::ServiceClient dynamic_tf_publisher_client;

  std::string marker_name;
  std::string server_name;
  std::string base_frame;
  std::string move_base_frame;
  std::string target_frame;


  boost::shared_ptr<ModelInterface> model;
  std::string model_name_;
  std::string frame_id_;
  std::string model_file_;
  geometry_msgs::Pose root_pose_;

  struct graspPoint{
    graspPoint(){
      displayMoveMarker = false;
      displayGraspPoint = false;
    }
    bool displayGraspPoint;
    bool displayMoveMarker;
    geometry_msgs::Pose pose;
  };


  struct linkProperty{
    linkProperty(){
      displayMoveMarker = false;
    }
    bool displayMoveMarker;
    graspPoint gp;
    //pose from frame_id
    geometry_msgs::Pose pose;
    geometry_msgs::Pose origin;
  };
  map<string, linkProperty> linkMarkerMap;
};

geometry_msgs::Pose getPose( XmlRpc::XmlRpcValue val);
double getXmlValue( XmlRpc::XmlRpcValue val );
