#include <jsk_interactive_marker/transformable_interactive_server.h>

using namespace jsk_interactive_marker;

TransformableInteractiveServer::TransformableInteractiveServer():n_(new ros::NodeHandle("~")){
  n_->param("display_interactive_manipulator", display_interactive_manipulator_, true);
  n_->param("torus_udiv", torus_udiv_, 20);
  n_->param("torus_vdiv", torus_vdiv_, 20);
  tf_listener_.reset(new tf::TransformListener);
  setpose_sub_ = n_->subscribe("set_pose", 1, &TransformableInteractiveServer::setPose, this);
  setcolor_sub_ = n_->subscribe("set_color", 1, &TransformableInteractiveServer::setColor, this);

  set_r_sub_ = n_->subscribe("set_radius", 1, &TransformableInteractiveServer::setRadius, this);
  set_sm_r_sub_ = n_->subscribe("set_small_radius", 1, &TransformableInteractiveServer::setSmallRadius, this);
  set_x_sub_ = n_->subscribe("set_x", 1, &TransformableInteractiveServer::setX, this);
  set_y_sub_ = n_->subscribe("set_y", 1, &TransformableInteractiveServer::setY, this);
  set_z_sub_ = n_->subscribe("set_z", 1, &TransformableInteractiveServer::setZ, this);

  addpose_sub_ = n_->subscribe("add_pose", 1, &TransformableInteractiveServer::addPose, this);
  addpose_relative_sub_ = n_->subscribe("add_pose_relative", 1, &TransformableInteractiveServer::addPoseRelative, this);

  setrad_sub_ = n_->subscribe("set_radius", 1, &TransformableInteractiveServer::setRadius, this);

  focus_text_pub_ = n_->advertise<jsk_rviz_plugins::OverlayText>("focus_marker_name", 1);
  focus_pose_pub_ = n_->advertise<jsk_rviz_plugins::OverlayText>("focus_marker_pose", 1);

  get_pose_srv_ = n_->advertiseService("get_pose", &TransformableInteractiveServer::getPoseService, this);
  get_type_srv_ = n_->advertiseService("get_type", &TransformableInteractiveServer::getTypeService, this);
  set_dimensions_srv =  n_->advertiseService("set_dimensions", &TransformableInteractiveServer::setDimensionsService, this);
  get_dimensions_srv =  n_->advertiseService("get_dimensions", &TransformableInteractiveServer::getDimensionsService, this);
  marker_dimensions_pub_ = n_->advertise<jsk_interactive_marker::MarkerDimensions>("marker_dimensions", 1);
  request_marker_operate_srv_ = n_->advertiseService("request_marker_operate", &TransformableInteractiveServer::requestMarkerOperateService, this);

  config_srv_ = boost::make_shared <dynamic_reconfigure::Server<InteractiveSettingConfig> > (*n_);
  dynamic_reconfigure::Server<InteractiveSettingConfig>::CallbackType f =
    boost::bind (&TransformableInteractiveServer::configCallback, this, _1, _2);
  config_srv_->setCallback (f);

  server_ = new interactive_markers::InteractiveMarkerServer("simple_marker");
}

TransformableInteractiveServer::~TransformableInteractiveServer()
{
  for (std::map<string, TransformableObject* >::iterator itpairstri = transformable_objects_map_.begin(); itpairstri != transformable_objects_map_.end(); itpairstri++) {
    delete itpairstri->second;
  }
  transformable_objects_map_.clear();
}

void TransformableInteractiveServer::configCallback(InteractiveSettingConfig &config, uint32_t level)
  {
    boost::mutex::scoped_lock lock(mutex_);
    for (std::map<string, TransformableObject* >::iterator itpairstri = transformable_objects_map_.begin(); itpairstri != transformable_objects_map_.end(); itpairstri++) {
      TransformableObject* tobject = itpairstri->second;
      tobject->setInteractiveMarkerSetting(config);
      updateTransformableObject(tobject);
    }
  }


void TransformableInteractiveServer::processFeedback(
                                                     const visualization_msgs::InteractiveMarkerFeedbackConstPtr &feedback )
{
  switch ( feedback->event_type )
    {
    case visualization_msgs::InteractiveMarkerFeedback::MOUSE_DOWN:
      focus_object_marker_name_ = feedback->marker_name;
      focusTextPublish();
      focusPosePublish();
      break;

    case visualization_msgs::InteractiveMarkerFeedback::POSE_UPDATE:
      TransformableObject* tobject = transformable_objects_map_[feedback->marker_name.c_str()];
      if(tobject){
        try {
          geometry_msgs::PoseStamped transformed_pose_stamped, input_pose_stamped;
          input_pose_stamped.header = feedback->header;
          input_pose_stamped.pose = feedback->pose;
          if (tf_listener_->waitForTransform(tobject->getFrameId(),
                                             feedback->header.frame_id, feedback->header.stamp, ros::Duration(1.0))) {
            tf_listener_->transformPose(tobject->getFrameId(), input_pose_stamped, transformed_pose_stamped);
            tobject->setPose(transformed_pose_stamped.pose);
          }
          else {
            ROS_ERROR("failed to lookup transform %s -> %s", tobject->getFrameId().c_str(), 
                      feedback->header.frame_id.c_str());
          }
        }
        catch (tf2::ConnectivityException &e)
        {
          ROS_ERROR("Transform error: %s", e.what());
        }
        catch (tf2::InvalidArgumentException &e)
        {
          ROS_ERROR("Transform error: %s", e.what());
        }
        tobject->setPose(feedback->pose);
      }else
        ROS_ERROR("Invalid ObjectId Request Received %s", feedback->marker_name.c_str());
      break;
    }
}

void TransformableInteractiveServer::setColor(std_msgs::ColorRGBA msg)
{
  if (transformable_objects_map_.find(focus_object_marker_name_) == transformable_objects_map_.end()) { return; }
  TransformableObject* tobject = transformable_objects_map_[focus_object_marker_name_];
  tobject->setRGBA(msg.r, msg.g, msg.b, msg.a);
  updateTransformableObject(tobject);
}

void TransformableInteractiveServer::setRadius(std_msgs::Float32 msg)
{
  if (transformable_objects_map_.find(focus_object_marker_name_) == transformable_objects_map_.end()) { return; }
  TransformableObject* tobject = transformable_objects_map_[focus_object_marker_name_];
  if(tobject->setRadius(msg)){
    updateTransformableObject(tobject);
    publishMarkerDimensions();
  }
}

void TransformableInteractiveServer::setSmallRadius(std_msgs::Float32 msg)
{
  if (transformable_objects_map_.find(focus_object_marker_name_) == transformable_objects_map_.end()) { return; }
  TransformableObject* tobject = transformable_objects_map_[focus_object_marker_name_];
  if(tobject->setSmallRadius(msg)){
    updateTransformableObject(tobject);
    publishMarkerDimensions();
  }
}

void TransformableInteractiveServer::setX(std_msgs::Float32 msg)
{
  if (transformable_objects_map_.find(focus_object_marker_name_) == transformable_objects_map_.end()) { return; }
  TransformableObject* tobject = transformable_objects_map_[focus_object_marker_name_];
  if(tobject->setX(msg)){
    updateTransformableObject(tobject);
    publishMarkerDimensions();
  }
}

void TransformableInteractiveServer::setY(std_msgs::Float32 msg)
{
  if (transformable_objects_map_.find(focus_object_marker_name_) == transformable_objects_map_.end()) { return; }
  TransformableObject* tobject = transformable_objects_map_[focus_object_marker_name_];
  if(tobject->setY(msg)){
    updateTransformableObject(tobject);
    publishMarkerDimensions();
  }
}

void TransformableInteractiveServer::setZ(std_msgs::Float32 msg)
{
  if (transformable_objects_map_.find(focus_object_marker_name_) == transformable_objects_map_.end()) { return; }
  TransformableObject* tobject = transformable_objects_map_[focus_object_marker_name_];
  if(tobject->setZ(msg)){
    updateTransformableObject(tobject);
    publishMarkerDimensions();
  }
}

void TransformableInteractiveServer::updateTransformableObject(TransformableObject* tobject)
{
  visualization_msgs::InteractiveMarker int_marker = tobject->getInteractiveMarker();
  server_->insert(int_marker, boost::bind( &TransformableInteractiveServer::processFeedback,this, _1));
  server_->applyChanges();
}

void TransformableInteractiveServer::setPose(geometry_msgs::PoseStamped msg){
  if (transformable_objects_map_.find(focus_object_marker_name_) == transformable_objects_map_.end()) { return; }
  TransformableObject* tobject =  transformable_objects_map_[focus_object_marker_name_];
  try {
    geometry_msgs::PoseStamped transformed_pose_stamped;
    if (tf_listener_->waitForTransform(tobject->getFrameId(), msg.header.frame_id, msg.header.stamp, ros::Duration(1.0))) {
      tf_listener_->transformPose(tobject->getFrameId(), msg, transformed_pose_stamped);
      tobject->setPose(transformed_pose_stamped.pose);
      server_->setPose(focus_object_marker_name_, msg.pose, msg.header);
      server_->applyChanges();
    }
    else {
      ROS_ERROR("failed to lookup transform %s -> %s", tobject->getFrameId().c_str(), msg.header.frame_id.c_str());
    }
  }
  catch (tf2::ConnectivityException &e)
  {
    ROS_ERROR("Transform error: %s", e.what());
  }
  catch (tf2::InvalidArgumentException &e)
  {
    ROS_ERROR("Transform error: %s", e.what());
  }
}

bool TransformableInteractiveServer::getPoseService(jsk_interactive_marker::GetPose::Request &req,jsk_interactive_marker::GetPose::Response &res)
{
  TransformableObject* tobject;
  if(req.target_name.compare(std::string("")) == 0){
    if (transformable_objects_map_.find(focus_object_marker_name_) == transformable_objects_map_.end()) { return true; }
    tobject = transformable_objects_map_[focus_object_marker_name_];
      res.pose = tobject->getPose();
  }else{
    if (transformable_objects_map_.find(req.target_name) == transformable_objects_map_.end()) { return true; }
    tobject = transformable_objects_map_[req.target_name];
    res.pose = tobject->getPose();
  }
  return true;
}

bool TransformableInteractiveServer::getTypeService(jsk_interactive_marker::GetType::Request &req,jsk_interactive_marker::GetType::Response &res)
{
  TransformableObject* tobject;
  if(req.target_name.compare(std::string("")) == 0){
    if (transformable_objects_map_.find(focus_object_marker_name_) == transformable_objects_map_.end()) { return true; }
    tobject = transformable_objects_map_[focus_object_marker_name_];
    res.type = tobject->type_;
  }else{
    if (transformable_objects_map_.find(req.target_name) == transformable_objects_map_.end()) { return true; }
    tobject = transformable_objects_map_[req.target_name];
    res.type = tobject->type_;
  }
  return true;
}

bool TransformableInteractiveServer::setDimensionsService(jsk_interactive_marker::SetMarkerDimensions::Request &req,jsk_interactive_marker::SetMarkerDimensions::Response &res)
{
  TransformableObject* tobject;
  if(req.target_name.compare(std::string("")) == 0){
    if (transformable_objects_map_.find(focus_object_marker_name_) == transformable_objects_map_.end()) { return true; }
    tobject = transformable_objects_map_[focus_object_marker_name_];
  }else{
    if (transformable_objects_map_.find(req.target_name) == transformable_objects_map_.end()) { return true; }
    tobject = transformable_objects_map_[req.target_name];
  }
  if (tobject) {
    if (tobject->getType() == jsk_rviz_plugins::TransformableMarkerOperate::BOX) {
      tobject->setXYZ(req.dimensions.x, req.dimensions.y, req.dimensions.z);
    } else if (tobject->getType() == jsk_rviz_plugins::TransformableMarkerOperate::CYLINDER) {
      tobject->setRZ(req.dimensions.radius, req.dimensions.z);
    } else if (tobject->getType() == jsk_rviz_plugins::TransformableMarkerOperate::TORUS) {
      tobject->setRSR(req.dimensions.radius, req.dimensions.small_radius);
    }
    publishMarkerDimensions();
  }
  return true;
}

bool TransformableInteractiveServer::getDimensionsService(jsk_interactive_marker::GetMarkerDimensions::Request &req,jsk_interactive_marker::GetMarkerDimensions::Response &res)
{
  TransformableObject* tobject;
  if(req.target_name.compare(std::string("")) == 0){
    if (transformable_objects_map_.find(focus_object_marker_name_) == transformable_objects_map_.end()) { return true; }
    tobject = transformable_objects_map_[focus_object_marker_name_];
  }else{
    if (transformable_objects_map_.find(req.target_name) == transformable_objects_map_.end()) { return true; }
    tobject = transformable_objects_map_[req.target_name];
  }
  if (tobject) {
    if (tobject->getType() == jsk_rviz_plugins::TransformableMarkerOperate::BOX) {
      tobject->getXYZ(res.dimensions.x, res.dimensions.y, res.dimensions.z);
    } else if (tobject->getType() == jsk_rviz_plugins::TransformableMarkerOperate::CYLINDER) {
      tobject->getRZ(res.dimensions.radius, res.dimensions.z);
    } else if (tobject->getType() == jsk_rviz_plugins::TransformableMarkerOperate::TORUS) {
      tobject->getRSR(res.dimensions.radius, res.dimensions.small_radius);
    }
  }
  return true;
}

void TransformableInteractiveServer::publishMarkerDimensions()
{
  TransformableObject* tobject;
  if (transformable_objects_map_.find(focus_object_marker_name_) == transformable_objects_map_.end()) { return; }
  tobject = transformable_objects_map_[focus_object_marker_name_];
  if (tobject) {
    jsk_interactive_marker::MarkerDimensions marker_dimensions;
    if (tobject->getType() == jsk_rviz_plugins::TransformableMarkerOperate::BOX) {
      tobject->getXYZ(marker_dimensions.x, marker_dimensions.y, marker_dimensions.z);
    } else if (tobject->getType() == jsk_rviz_plugins::TransformableMarkerOperate::CYLINDER) {
      tobject->getRZ(marker_dimensions.radius, marker_dimensions.z);
    } else if (tobject->getType() == jsk_rviz_plugins::TransformableMarkerOperate::TORUS) {
      tobject->getRSR(marker_dimensions.radius, marker_dimensions.small_radius);
    }
    marker_dimensions.type = tobject->type_;
    marker_dimensions_pub_.publish(marker_dimensions);
  }
}
  


bool TransformableInteractiveServer::requestMarkerOperateService(jsk_rviz_plugins::RequestMarkerOperate::Request &req,jsk_rviz_plugins::RequestMarkerOperate::Response &res)
{
  switch(req.operate.action){
  case jsk_rviz_plugins::TransformableMarkerOperate::INSERT:
    if (req.operate.type == jsk_rviz_plugins::TransformableMarkerOperate::BOX) {
      insertNewBox(req.operate.frame_id, req.operate.name, req.operate.description);
    } else if (req.operate.type == jsk_rviz_plugins::TransformableMarkerOperate::CYLINDER) {
      insertNewCylinder(req.operate.frame_id, req.operate.name, req.operate.description);
    } else if (req.operate.type == jsk_rviz_plugins::TransformableMarkerOperate::TORUS) {
      insertNewTorus(req.operate.frame_id, req.operate.name, req.operate.description);
    }
    return true;
    break;
  case jsk_rviz_plugins::TransformableMarkerOperate::ERASE:
    eraseObject(req.operate.name);
    return true;
    break;
  case jsk_rviz_plugins::TransformableMarkerOperate::ERASEALL:
    eraseAllObject();
    return true;
    break;
  case jsk_rviz_plugins::TransformableMarkerOperate::ERASEFOCUS:
    eraseFocusObject();
    return true;
    break;
  };
  return false;
}

void TransformableInteractiveServer::addPose(geometry_msgs::Pose msg){
  if (transformable_objects_map_.find(focus_object_marker_name_) == transformable_objects_map_.end()) { return; }
  TransformableObject* tobject = transformable_objects_map_[focus_object_marker_name_];
  tobject->addPose(msg,false);
  updateTransformableObject(tobject);
}

void TransformableInteractiveServer::addPoseRelative(geometry_msgs::Pose msg){
  if (transformable_objects_map_.find(focus_object_marker_name_) == transformable_objects_map_.end()) { return; }
  TransformableObject* tobject = transformable_objects_map_[focus_object_marker_name_];
  tobject->addPose(msg,true);
  updateTransformableObject(tobject);
}

void TransformableInteractiveServer::focusTextPublish(){
  jsk_rviz_plugins::OverlayText focus_text;
  focus_text.text = focus_object_marker_name_;
  focus_text.top = 0;
  focus_text.left = 0;
  focus_text.width = 300;
  focus_text.height = 50;
  focus_text.bg_color.r = 0.9;
  focus_text.bg_color.b = 0.9;
  focus_text.bg_color.g = 0.9;
  focus_text.bg_color.a = 0.1;
  focus_text.fg_color.r = 0.3;
  focus_text.fg_color.g = 0.3;
  focus_text.fg_color.b = 0.8;
  focus_text.fg_color.a = 1;
  focus_text.line_width = 1;
  focus_text.text_size = 30;
  focus_text_pub_.publish(focus_text);
}

void TransformableInteractiveServer::focusPosePublish(){
  geometry_msgs::Pose target_pose;
  std::stringstream ss;
  if (transformable_objects_map_.find(focus_object_marker_name_) != transformable_objects_map_.end()) {
    target_pose = transformable_objects_map_[focus_object_marker_name_]->getPose();
    ss << "Pos x: " << target_pose.position.x  << " y: " << target_pose.position.y << " z: " << target_pose.position.z
       << std::endl
       << "Ori x: " << target_pose.orientation.x << " y: " << target_pose.orientation.y << " z: " << target_pose.orientation.z << " w: " << target_pose.orientation.w;
  }

  jsk_rviz_plugins::OverlayText focus_pose;
  focus_pose.text = ss.str();
  focus_pose.top = 50;
  focus_pose.left = 0;
  focus_pose.width = 500;
  focus_pose.height = 50;
  focus_pose.bg_color.r = 0.9;
  focus_pose.bg_color.b = 0.9;
  focus_pose.bg_color.g = 0.9;
  focus_pose.bg_color.a = 0.1;
  focus_pose.fg_color.r = 0.8;
  focus_pose.fg_color.g = 0.3;
  focus_pose.fg_color.b = 0.3;
  focus_pose.fg_color.a = 1;
  focus_pose.line_width = 1;
  focus_pose.text_size = 15;
  focus_pose_pub_.publish(focus_pose);
}

void TransformableInteractiveServer::insertNewBox(std::string frame_id, std::string name, std::string description)
{
  TransformableBox* transformable_box = new TransformableBox(0.45, 0.45, 0.45, 0.5, 0.5, 0.5, 1.0, frame_id, name, description);
  insertNewObject(transformable_box, name);
}

void TransformableInteractiveServer::insertNewCylinder( std::string frame_id, std::string name, std::string description)
{
  TransformableCylinder* transformable_cylinder = new TransformableCylinder(0.45, 0.45, 0.5, 0.5, 0.5, 1.0, frame_id, name, description);
  insertNewObject(transformable_cylinder, name);
}

void TransformableInteractiveServer::insertNewTorus( std::string frame_id, std::string name, std::string description)
{
  TransformableTorus* transformable_torus = new TransformableTorus(0.45, 0.2, torus_udiv_, torus_vdiv_, 0.5, 0.5, 0.5, 1.0, frame_id, name, description);
  insertNewObject(transformable_torus, name);
}

void TransformableInteractiveServer::insertNewObject( TransformableObject* tobject , std::string name )
{
  SetInitialInteractiveMarkerConfig(tobject);
  visualization_msgs::InteractiveMarker int_marker = tobject->getInteractiveMarker();
  transformable_objects_map_[name] = tobject;
  server_->insert(int_marker, boost::bind( &TransformableInteractiveServer::processFeedback,this, _1));
  server_->applyChanges();

  focus_object_marker_name_ = name;
}

void TransformableInteractiveServer::SetInitialInteractiveMarkerConfig( TransformableObject* tobject )
{
  InteractiveSettingConfig config;
  config.display_interactive_manipulator = display_interactive_manipulator_;
  tobject->setInteractiveMarkerSetting(config);
}

void TransformableInteractiveServer::eraseObject( std::string name )
{
  server_->erase(name);
  server_->applyChanges();
  if (focus_object_marker_name_.compare(name) == 0) {
    focus_object_marker_name_ = "";
    focusTextPublish();
    focusPosePublish();
  }
  delete transformable_objects_map_[name];
  transformable_objects_map_.erase(name);
}

void TransformableInteractiveServer::eraseAllObject()
{
  for (std::map<string, TransformableObject* >::iterator itpairstri = transformable_objects_map_.begin(); itpairstri != transformable_objects_map_.end(); itpairstri++) {
    eraseObject(itpairstri->first);
  }
}

void TransformableInteractiveServer::eraseFocusObject()
{
  eraseObject(focus_object_marker_name_);
}

void TransformableInteractiveServer::run(){
  ros::spin();
}
