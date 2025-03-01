/*
 * Copyright (c) 2019, the neonavigation authors
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
 *     * Neither the name of the copyright holder nor the names of its
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

#include <ros/ros.h>
#include <nav_msgs/OccupancyGrid.h>
#include <nav_msgs/Path.h>

#include <gtest/gtest.h>

class DebugOutputsTest : public ::testing::Test
{
public:
  DebugOutputsTest()
    : cnt_path_(0)
  {
    sub_path_ = nh_.subscribe("path", 1, &DebugOutputsTest::cbPath, this);
    sub_hysteresis_ = nh_.subscribe("/planner_3d/hysteresis_map", 1, &DebugOutputsTest::cbHysteresis, this);
    sub_remembered_ = nh_.subscribe("/planner_3d/remembered_map", 1, &DebugOutputsTest::cbRemembered, this);

    // Wait until receiving some paths
    while (ros::ok())
    {
      ros::Duration(0.1).sleep();
      ros::spinOnce();
      if (cnt_path_ > 10)
        break;
    }
  }

protected:
  void cbHysteresis(const nav_msgs::OccupancyGrid::ConstPtr& msg)
  {
    map_hysteresis_ = msg;
  }
  void cbRemembered(const nav_msgs::OccupancyGrid::ConstPtr& msg)
  {
    map_remembered_ = msg;
  }
  void cbPath(const nav_msgs::Path::ConstPtr& msg)
  {
    if (msg->poses.size() > 0)
      ++cnt_path_;
  }

  ros::NodeHandle nh_;
  nav_msgs::OccupancyGrid::ConstPtr map_hysteresis_;
  nav_msgs::OccupancyGrid::ConstPtr map_remembered_;
  ros::Subscriber sub_path_;
  ros::Subscriber sub_hysteresis_;
  ros::Subscriber sub_remembered_;
  int cnt_path_;
};

struct PositionAndValue
{
  int x;
  int y;
  char value;
};

TEST_F(DebugOutputsTest, Hysteresis)
{
  ASSERT_TRUE(static_cast<bool>(map_hysteresis_));

  // Robot is at (25, 4) and goal is at (10, 4)
  const PositionAndValue data_set[] =
      {
        { 25, 4, 0 },
        { 20, 4, 0 },
        { 15, 4, 0 },
        { 10, 4, 0 },
        { 25, 1, 100 },
        { 20, 1, 100 },
        { 15, 1, 100 },
        { 10, 1, 100 },
        { 25, 7, 100 },
        { 20, 7, 100 },
        { 15, 7, 100 },
        { 10, 7, 100 },
      };
  for (auto data : data_set)
  {
    const size_t addr = data.x + data.y * map_hysteresis_->info.width;
    EXPECT_EQ(data.value, map_hysteresis_->data[addr]) << "x: " << data.x << ", y: " << data.y;
  }
}

TEST_F(DebugOutputsTest, Remembered)
{
  ASSERT_TRUE(static_cast<bool>(map_remembered_));

  // Robot is at (25, 5) and obstacles are placed at
  // (0, 0)-(31, 0) and (18, 10)-(31, 10).
  // Costmap is expanded by 1 grid.
  const PositionAndValue data_set[] =
      {
        { 17, 0, 100 },   // occupied
        { 17, 1, 100 },   // expanded
        { 17, 2, 0 },     // free
        { 17, 8, 0 },     // free
        { 17, 9, 100 },   // expanded
        { 17, 10, 100 },  // occupied
      };
  for (auto data : data_set)
  {
    const size_t addr = data.x + data.y * map_remembered_->info.width;
    EXPECT_EQ(data.value, map_remembered_->data[addr]) << "x: " << data.x << ", y: " << data.y;
  }
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  ros::init(argc, argv, "test_debug_outputs");

  return RUN_ALL_TESTS();
}
