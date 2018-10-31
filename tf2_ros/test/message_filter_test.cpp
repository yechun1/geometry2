/*
 * Copyright (c) 2014, Open Source Robotics Foundation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/point_stamped.hpp>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <message_filters/subscriber.h>
#include <message_filters/simple_filter.h>
#include <message_filters/message_traits.h>
#include <tf2_ros/buffer.h>
#include <tf2_ros/message_filter.h>
#include <tf2_ros/static_transform_broadcaster.h>
#include <tf2_ros/transform_listener.h>

#include <gtest/gtest.h>


void spin_for_a_second(rclcpp::Node::SharedPtr node)
{
  //ros::spinOnce();
  rclcpp::spin_some(node);
  for (uint8_t i = 0; i < 10; ++i)
  {
    usleep(100);
    //ros::spinOnce();
    rclcpp::spin_some(node);
  }
}

bool filter_callback_fired = false;
void filter_callback(const geometry_msgs::msg::PointStamped& msg)
{
  filter_callback_fired = true;
}

TEST(tf2_ros_message_filter, multiple_frames_and_time_tolerance)
{
  //ros::NodeHandle nh;
  auto node = rclcpp::Node::make_shared("tf2_ros_message_filter");

  message_filters::Subscriber<geometry_msgs::msg::PointStamped> sub;
  sub.subscribe(node, "point");

  rclcpp::Clock::SharedPtr clock = std::make_shared<rclcpp::Clock>(RCL_SYSTEM_TIME);
  tf2_ros::Buffer buffer(clock);
  tf2_ros::TransformListener tfl(buffer);
  tf2_ros::MessageFilter<geometry_msgs::msg::PointStamped> filter(buffer, "map", 10);
  filter.connectInput(sub);
  filter.registerCallback(&filter_callback);
  // Register multiple target frames
  std::vector<std::string> frames;
  frames.push_back("odom");
  frames.push_back("map");
  filter.setTargetFrames(frames);
  // Set a non-zero time tolerance
  filter.setTolerance(tf2::durationFromSec(1.0));

  // Publish static transforms so the frame transformations will always be valid
  tf2_ros::StaticTransformBroadcaster tfb(node);
  geometry_msgs::msg::TransformStamped map_to_odom;
  map_to_odom.header.stamp = rclcpp::Time(0, 0);
  map_to_odom.header.frame_id = "map";
  map_to_odom.child_frame_id = "odom";
  map_to_odom.transform.translation.x = 0.0;
  map_to_odom.transform.translation.y = 0.0;
  map_to_odom.transform.translation.z = 0.0;
  map_to_odom.transform.rotation.x = 0.0;
  map_to_odom.transform.rotation.y = 0.0;
  map_to_odom.transform.rotation.z = 0.0;
  map_to_odom.transform.rotation.w = 1.0;
  tfb.sendTransform(map_to_odom);

  geometry_msgs::msg::TransformStamped odom_to_base;
  odom_to_base.header.stamp = rclcpp::Time(0, 0);
  odom_to_base.header.frame_id = "odom";
  odom_to_base.child_frame_id = "base";
  odom_to_base.transform.translation.x = 0.0;
  odom_to_base.transform.translation.y = 0.0;
  odom_to_base.transform.translation.z = 0.0;
  odom_to_base.transform.rotation.x = 0.0;
  odom_to_base.transform.rotation.y = 0.0;
  odom_to_base.transform.rotation.z = 0.0;
  odom_to_base.transform.rotation.w = 1.0;
  tfb.sendTransform(odom_to_base);

  // Publish a Point message in the "base" frame
  rclcpp::Publisher<geometry_msgs::msg::PointStamped>::SharedPtr pub;
  pub =  node->create_publisher<geometry_msgs::msg::PointStamped>("point");
  geometry_msgs::msg::PointStamped point;
  point.header.stamp = rclcpp::Clock().now();
  point.header.frame_id = "base";
  pub->publish(point);

  // make sure it arrives
  spin_for_a_second(node);

  // The filter callback should have been fired because all required transforms are available
  ASSERT_TRUE(filter_callback_fired);
}

int main(int argc, char **argv){
  testing::InitGoogleTest(&argc, argv);
  //ros::init(argc, argv, "tf2_ros_message_filter");
  rclcpp::init(argc, argv);
  return RUN_ALL_TESTS();
}
