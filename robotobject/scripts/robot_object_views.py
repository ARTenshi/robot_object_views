#!/usr/bin/env python
# Copyright (C) 2019 Toyota Motor Corporation

import random
import math
import numpy as np
from scipy.spatial.distance import pdist
import argparse

import rospy
import sys
import os
import time
import string
import warnings
import re

import tf2_ros
import traceback

from gazebo_ros import gazebo_interface

from gazebo_msgs.msg import *
from gazebo_msgs.srv import *
from std_srvs.srv import Empty
from geometry_msgs.msg import Point, Pose, Quaternion, Twist, Wrench
import tf.transformations as tft

#hsr libraries
import hsrb_interface

from hsrb_interface import geometry
from geometry_msgs.msg import Vector3Stamped, Pose, PoseStamped, PointStamped
from geometry_msgs.msg import WrenchStamped, Twist

import cv2
from cv_bridge import CvBridge, CvBridgeError
from sensor_msgs.msg import Image

robot = hsrb_interface.Robot()
whole_body = robot.get("whole_body")
omni_base = robot.get("omni_base") #Standard initialisation (Toyota)
gripper = robot.get('gripper')
tf_buffer = robot._get_tf2_buffer()

#erasers libraries
from erasers_nav_msgs.srv import GetObjectsCentroid

whole_body.move_to_neutral()
rospy.loginfo('initializing...')
rospy.sleep(3)

_rgb_mat = 0
_rgb = False

_path_xml = "/home/roboworks/ycb_ws/src/robot_object_views/robotobject/models/MODEL_NAME/model-1_4.sdf"
_path_model = "/home/roboworks/ycb_ws/src/robot_object_views/robotobject/models"

model_database_template = """<sdf version="1.4">
  <world name="default">
    <include>
      <uri>model://MODEL_NAME</uri>
    </include>
  </world>
</sdf>"""

center = [2.0, 2.0, 0.0]

delta_world_map = [-4.93, -6.555, 0.0]

furniture = {
    "items": [
        "bed",
        "bed_table",
        "chair",
        "counter_wagon",
        "hcr_shelf_1",
        "hcr_shelf_2",
        "high_chair",
        "high_shelf",
        "high_shelf",
        "high_table",
        "kitchen_chair",
        "kitchen_lowtable",
        "kitchen_table",
        "living_sideboard",
        "office_desk",
        "sink-fix",
        "sofa-fix",
        "sofa-fix",
        "tvboard",
        "tvboard",
        "wagon"
    ],

    "size": [ 
        [0.59,1.95, 0.40],	# bed
        [0.398,0.898,0.625],	# bed_table
        [0.45, 0.50, 0.50],	# chair
        [0.45, 1.190, 0.84],	# counter_wagon
        [0.295, 0.414, 0.33],	# hcr_shelf_1
        [0.295, 0.414, 0.60],	# hcr_shelf_2
        [0.35, 0.35, 0.84],	# high_chair
        [0.295, 0.414, 0.37],	# high_shelf_1
        [0.295, 0.414, 0.66],	# high_shelf_2
        [0.5, 2.3, 1.05],	# high_table
        [0.60, 0.52, 0.42],	# kitchen_chair
        [0.68, 1.13, 0.41],	# kitchen_lowtable
        [0.68, 1.13, 0.71],	# kitchen_table
        [0.44, 1.15, 0.62],	# living_sideboard
        [0.6, 1.2, 0.71],	# office_desk
        [0.65, 2.55, 0.86],	# sink-fix
        [0.90, 1.25, 0.27],	# sofa-fix
        [0.90, 1.25, 0.27],	# sofa-fix_lateral
        [0.29, 0.415, 0.3725],	# tvboard_bottom
        [0.29, 0.415, 0.66],	# tvboard_first
        [0.44, 0.445, 0.675]	# wagon
    ],

    "orientation": [ #[roll, pitch, yaw]
        [0.0, 0.0, 0.0],	# bed
        [0.0, 0.0, 0.0],	# bed_table
        [1.57, 0.0, 0.0],	# chair
        [0.0, 0.0, 0.0],	# counter_wagon
        [0.0, 0.0, 0.0],	# hcr_shelf_1
        [0.0, 0.0, 0.0],	# hcr_shelf_2
        [0.0, 0.0, 0.0],	# high_chair
        [0.0, 0.0, 0.0],	# high_shelf_1
        [0.0, 0.0, 0.0],	# high_shelf_2
        [0.0, 0.0, 0.0],	# high_table
        [0.0, 0.0, 0.0],	# kitchen_chair
        [0.0, 0.0, 0.0],	# kitchen_lowtable
        [0.0, 0.0, 0.0],	# kitchen_table
        [0.0, 0.0, 0.0],	# living_sideboard
        [0.0, 0.0, 0.0],	# office_desk
        [0.0, 0.0, 0.0],	# sink-fix
        [0.0, 0.0, 0.0],	# sofa-fix
        [0.0, 0.0, 0.0],	# sofa-fix_lateral
        [0.0, 0.0, 0.0],	# tvboard_bottom
        [0.0, 0.0, 0.0],	# tvboard_first
        [0.0, 0.0, 0.0]		# wagon
    ],

    "origin": [ 
        [0.59, 1.95, 0.0],	# bed
        [0.20, 0.0, 0.0],	# bed_table
        [0.225, 0.25, 0.0],	# chair 
        [0.45, 0.0, 0.0],	# counter_wagon 
        [0.295, 0.0, 1.196],	# hcr_shelf_1
        [0.295, 0.0, 1.196],	# hcr_shelf_2
        [0.175, 0.175, 0.0],	# high_chair
        [0.295, 0.0, 0.0],	# high_shelf_1
        [0.295, 0.0, 0.0],	# high_shelf_2
        [0.25, 1.15, 0.0],	# high_table
        [0.30, 0.26, 0.0],	# kitchen_chair
        [0.34, 0.565, 0.0],	# kitchen_lowtable
        [0.34, 0.565, 0.0],	# kitchen_table
        [0.44, 0.0, 0.0],	# living_sideboard
        [0.6, 0.0, 0.0],	# office_desk
        [0.65, 0.0, 0.0],	# sink-fix
        [0.45, 0.63, 0.0],	# sofa-fix
        [0.325, 0.415, 0.0],	# sofa-fix_lateral
        [0.29, 0.0, 1.2315],	# tvboard_bottom
        [0.29, 0.0, 1.2315],	# tvboard_first
        [0.44, 0.0, 0.0]	# wagon
    ],

    "views": [ #[front, left, back, right]
        [1, 1, 1, 1],		# bed
        [1, 1, 1, 1],		# bed_table
        [1, 1, 0, 1],		# chair 
        [1, 1, 1, 1],		# counter_wagon 
        [0, 1, 0, 0],		# hcr_shelf_1
        [0, 1, 0, 0],		# hcr_shelf_2
        [1, 0, 1, 1],		# high_chair
        [0, 1, 0, 0],		# high_shelf_1
        [0, 1, 0, 0],		# high_shelf_2
        [1, 1, 1, 1],		# high_table
        [1, 1, 1, 0],		# kitchen_chair
        [1, 1, 1, 1],		# kitchen_lowtable
        [1, 1, 1, 1],		# kitchen_table
        [1, 1, 1, 1],		# living_sideboard
        [1, 1, 1, 1],		# office_desk
        [1, 1, 0, 0],		# sink-fix
        [0, 1, 0, 0],		# sofa-fix
        [0, 1, 0, 0],		# sofa-fix_lateral
        [0, 1, 0, 0],		# tvboard_bottom
        [0, 1, 0, 0],		# tvboard_first
        [1, 1, 1, 1]		# wagon
    ]
}

objects = {
    "items": [
        "002_master_chef_can",       # 000 food
        "003_cracker_box",           # 001 food
        "004_sugar_box",             # 002 food
        "005_tomato_soup_can",       # 003 food
        "006_mustard_bottle",        # 004 food
        "007_tuna_fish_can",         # 005 food
        "008_pudding_box",           # 006 food
        "009_gelatin_box",           # 007 food
        "010_potted_meat_can",       # 008 food
        "011_banana",                # 009 food
        "012_strawberry",            # 010 food
        "013_apple",                 # 011 food
        "014_lemon",                 # 012 food
        "015_peach",                 # 013 food
        "016_pear",                  # 014 food
        "017_orange",                # 015 food
        "018_plum",                  # 016 food
        "019_pitcher_base",          # 017 kitchen_items
        "021_bleach_cleanser",       # 018 kitchen_items
        "022_windex_bottle",         # 019 kitchen_items
        "024_bowl",                  # 020 kitchen_items
        "025_mug",                   # 021 kitchen_items
        "026_sponge",                # 022 kitchen_items
        "029_plate",                 # 023 kitchen_items
        "030_fork",                  # 024 kitchen_items
        "031_spoon",                 # 025 kitchen_items
        "033_spatula",               # 026 kitchen_items
        # missing: Pitcher lid, Wine glass
        "038_padlock",               # 027 tools
        "040_large_marker",          # 028 tools
        "050_medium_clamp",          # 029 tools
        "051_large_clamp",           # 030 tools
        "052_extra_large_clamp",     # 031 tools
        # missing: Small marker, Small clamp, Bolts, Nuts
        "053_mini_soccer_ball",      # 032 shape_items
        "054_softball",              # 033 shape_items
        "055_baseball",              # 034 shape_items
        "056_tennis_ball",           # 035 shape_items
        "057_racquetball",           # 036 shape_items
        "058_golf_ball",             # 037 shape_items
        "059_chain",                 # 038 shape_items
        "061_foam_brick",            # 039 shape_items
        "062_dice",                  # 040 shape_items
        "063-a_marbles",             # 041 shape_items
        "063-b_marbles",             # 042 shape_items
        "065-a_cups",                # 043 shape_items
        "065-b_cups",                # 044 shape_items
        "065-c_cups",                # 045 shape_items
        "065-d_cups",                # 046 shape_items
        "065-e_cups",                # 047 shape_items
        "065-f_cups",                # 048 shape_items
        "065-g_cups",                # 049 shape_items
        "065-h_cups",                # 050 shape_items
        "065-i_cups",                # 051 shape_items
        "065-j_cups",                # 052 shape_items
        # missing: Rope, Credit card blank
        "070-a_colored_wood_blocks", # 053 task_items
        "070-b_colored_wood_blocks", # 054 task_items
        "071_nine_hole_peg_test",    # 055 task_items
        "072-a_toy_airplane",        # 056 task_items
        "072-b_toy_airplane",        # 057 task_items
        "072-c_toy_airplane",        # 058 task_items
        "072-d_toy_airplane",        # 059 task_items
        "072-e_toy_airplane",        # 060 task_items
        "073-a_lego_duplo",          # 061 task_items
        "073-b_lego_duplo",          # 062 task_items
        "073-c_lego_duplo",          # 063 task_items
        "073-d_lego_duplo",          # 064 task_items
        "073-e_lego_duplo",          # 065 task_items
        "073-f_lego_duplo",          # 066 task_items
        "073-g_lego_duplo",          # 067 task_items
        "077_rubiks_cube",           # 068 task_items
        # missing: Black t-shirt, Timer, Magazine
    ]
}

def image_rect_color_cb(msg):
	global _rgb
	global _rgb_mat
	try:
            bridge_rgb = CvBridge()
            _rgb_mat = bridge_rgb.imgmsg_to_cv2(msg, msg.encoding)
	    _rgb = True

        except CvBridgeError as cv_bridge_exception:
            rospy.logerr(cv_bridge_exception)
	    _rgb = False

def spawn_furniture(gazebo_name, name, x, y, z, orientation):
    rospy.loginfo('Spawn: {0}'.format(name))
    initial_pose = Pose()
    initial_pose.position.x = x
    initial_pose.position.y = y
    initial_pose.position.z = z
    roll = orientation[0]
    pitch = orientation[1]
    yaw = orientation[2]
    q = tft.quaternion_from_euler(roll, pitch, yaw)
    initial_pose.orientation = Quaternion(q[0], q[1], q[2], q[3])

    model_xml = model_database_template.replace('MODEL_NAME', name)

    gazebo_interface.spawn_sdf_model_client(gazebo_name, model_xml, rospy.get_namespace(),
                                            initial_pose, "", "/gazebo")

def spawn_object(gazebo_name, name, x, y, z, yaw):
    rospy.loginfo('Spawn: {0}'.format(name))
    initial_pose = Pose()
    initial_pose.position.x = x
    initial_pose.position.y = y
    initial_pose.position.z = z
    roll = 0.0
    pitch = 0.0
    yaw = math.pi * (random.random() - 0.5)
    q = tft.quaternion_from_euler(roll, pitch, yaw)
    initial_pose.orientation = Quaternion(q[0], q[1], q[2], q[3])

    path_xml = _path_xml.replace('MODEL_NAME', name)

    with open(path_xml, "r") as f:
        model_xml = f.read()

    model_xml = model_xml.replace('PATH_TO_MODEL', _path_model)

    gazebo_interface.spawn_sdf_model_client(gazebo_name, model_xml, rospy.get_namespace(),
                                            initial_pose, "", "/gazebo")

if __name__ == '__main__':

    #rospy.init_node('spawn_objects')

    #ROS SERVICES
    #RGB image topic
    color_image_sub = rospy.Subscriber('/hsrb/head_rgbd_sensor/rgb/image_rect_color', Image, image_rect_color_cb)

    #Start ObjectFinder client
    rospy.wait_for_service('/erasers/navigation/object_finder_srv')
    get_objects_centroid = rospy.ServiceProxy('/erasers/navigation/object_finder_srv', GetObjectsCentroid)

    tf_buffer = tf2_ros.Buffer(rospy.Duration(5.))
    tf2_ros.TransformListener(tf_buffer)

    #SPAWN FURNITURE
    rospy.wait_for_service('/gazebo/delete_model')
    delete_model = rospy.ServiceProxy('/gazebo/delete_model', DeleteModel)

    frntr_idx = 20

    frntr_x = center[0] + (furniture['origin'][frntr_idx][0] - 0.5*furniture['size'][frntr_idx][0])
    frntr_y = center[1] + (furniture['origin'][frntr_idx][1] - 0.5*furniture['size'][frntr_idx][1])
    frntr_z = furniture['origin'][frntr_idx][2]

    spawn_furniture(furniture['items'][frntr_idx], furniture['items'][frntr_idx], frntr_x, frntr_y, frntr_z, furniture['orientation'][frntr_idx])

    #DEFINE PATH
    path = []

    delta_x = 0.0
    delta_y = 0.80
    pos_x = center[0] + delta_world_map[0] + delta_x
    pos_y = center[1] + delta_world_map[1] + 0.5*furniture['size'][frntr_idx][1] + delta_y
    pos_yaw = -1.57

    path.append([pos_x,pos_y,pos_yaw])

    delta_x = 0.0
    delta_y = 0.70
    pos_x = center[0] + delta_world_map[0] - 0.5*furniture['size'][frntr_idx][1] + delta_x
    pos_y = center[1] + delta_world_map[1] + 0.5*furniture['size'][frntr_idx][1] + delta_y
    pos_yaw = -1.05

    path.append([pos_x,pos_y,pos_yaw])

    delta_x = 0.0
    delta_y = 0.70
    pos_x = center[0] + delta_world_map[0] + 0.5*furniture['size'][frntr_idx][1] + delta_x
    pos_y = center[1] + delta_world_map[1] + 0.5*furniture['size'][frntr_idx][1] + delta_y
    pos_yaw = -2.09

    path.append([pos_x,pos_y,pos_yaw])

    for j in range(0, len(path)):
	#MOVE ROBOT CLOSE TO FURNITURE
	omni_base.go_abs(path[j][0], path[j][1], path[j][2])

	#MOVE HEAD TOWARDS OBJECT POSITION
	tilt = -0.36
	whole_body.move_to_go()
	whole_body.move_to_joint_positions({'head_pan_joint': 0.0, 'head_tilt_joint': tilt})

        #FIND OBJECT
	#for i in range(0, len(objects['items'])):
        for i in range(0, 5):

		#Spawn object
		item_x = random.uniform(center[0] - 0.025, center[0] + 0.025)
		item_y = random.uniform(center[1] - 0.025, center[1] + 0.025)
		item_z = furniture['size'][frntr_idx][2]
		item_yaw = math.pi * (random.random() - 0.5)

		model_name = objects['items'][i]

		spawn_object(model_name, model_name, item_x, item_y, item_z, item_yaw)

		rospy.sleep(2)

		##################
		#Find closest object centroid
		depth_min = 0.40
		depth_max = 2.50
		width_min = -0.40
		width_max = 0.40
		height_min = furniture['size'][frntr_idx][2] - 0.10
		height_max = furniture['size'][frntr_idx][2] + 0.10
		min_area = 25
		max_area = 100000

		plane = True
		bigplane = True
		vertical = False

		counter = 0
		while True:
			#Call service with parameter (depth_min, depth_max, width_min, width_max, height_min, height_max, min_area, max_area, plane, bigplane, vertical)
			#Output in camera coordinates WRT the robot
			#i_width positive to the right
			#j_height positive upwards
			#k_depth positive to the front
			#centroid = [i_width, j_height, k_depth]
			_objects = get_objects_centroid(depth_min, depth_max, width_min, width_max, height_min, height_max, min_area, max_area, plane, bigplane, vertical).objects

			counter += 1
			if _objects.isobject == True or counter == 2:
				break

		if _objects.isobject == False:
			print "No object found!"

			try:
			    delete_model(model_name)
			    print "Model %s removed successfuly"%model_name

			except rospy.ServiceException, e:
			    print "Service call failed: %s"%e

			continue

		while not _rgb:
			rospy.sleep(1)

		rgb_mat = _rgb_mat

		#Get closest object index
		obj_idx = -1
		n = _objects.n
		dist = 10000000
		for q in range(_objects.n):
			_dist = math.sqrt(math.pow(_objects.centroid[q], 2) + math.pow(_objects.centroid[2*n+q], 2))
			if (_dist < dist):
				dist = _dist
				obj_idx = q 

		Xmin = _objects.bbox[obj_idx]
		Ymin = _objects.bbox[n+obj_idx]
		Xmax = _objects.bbox[obj_idx] + _objects.bbox[2*n+obj_idx]
		Ymax = _objects.bbox[n+obj_idx] + _objects.bbox[3*n+obj_idx]

		cv2.rectangle(rgb_mat,
		              (int(Xmin), int(Ymin)),
		              (int(Xmax), int(Ymax)),
		              (255, 0, 0), 2)

		cv2.imshow("rgb", rgb_mat)
		cv2.waitKey(30)

		rospy.sleep(1)

		##################
		#Delete object
		try:
		    delete_model(model_name)
		    print "Model %s removed successfuly"%model_name

		except rospy.ServiceException, e:
		    print "Service call failed: %s"%e

    delete_model(furniture['items'][frntr_idx])

