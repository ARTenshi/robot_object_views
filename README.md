# Arm Control with MoveIt

Follow the diagrams to build the same arm configuration from [here](https://gbiggs.github.io/ros_moveit_rsj_tutorial/manipulators_and_moveit.html) but **don't install anything yet**; after you had build your arm, follow the installation instructions given bellow.

# Installation

**Note**: Be sure to plug the Arm controller before anything else.

## 1. MoveIt

### Ubuntu 16.04:

Follow the indications here:

```
sudo apt-get install ros-kinetic-moveit-*
```

### Ubuntu 18.04

Proceed as indicated in the following link:

```
sudo apt-get install ros-melodic-moveit-*
```


## 2. Get the Arm Control libraries:

### 2.1 Clone this repository

**Warning:** *You only need to do this once. If you have already created this repository in your local machine, pulling it again may cause a lost of your information.*

First, create a workspace:

```
cd ~
mkdir -p armcontrol_ws/src
cd armcontrol_ws
catkin_make
```

Then, clone this repository into the src folder:

```
cd ~/armcontrol_ws/src
git clone https://github.com/ARTenshi/armcontroler.git
cd ..
catkin_make
source devel/setup.bash
```

### 2.2 Test the code

Run the following command in a terminal:

```
roslaunch arm_poses arm_poses.launch
```

If you see any motor in red, restart the previous command.


Then, in another terminal enter these commands sequentially (wait for the arm to finish moving before entering the next command):

```
rostopic pub /arm_move_vertical std_msgs/Bool "true"
```

```
rostopic pub /arm_move_front std_msgs/Bool "true"
```

```
rostopic pub /arm_gripper_open std_msgs/Bool "true"
```

```
rostopic pub /arm_gripper_close std_msgs/Bool "true"
```

Alternatively, you can move every joint individualy as follows.

To move Motor 1, *shoulder_revolute* (z axis), in a separate terminal enter (**data: 0.0** is the final orientation in radians):

```
rostopic pub /shoulder_revolute_servo_controller/command std_msgs/Float64 "data: 0.0"
```

To move Motor 2, *shoulder_flex* (y axis), enter:

```
rostopic pub /shoulder_flex_servo_controller/command std_msgs/Float64 "data: 0.0"
```

To move Motor 3, *elbow_servo* (y axis), enter:

```
rostopic pub /elbow_servo_controller/command std_msgs/Float64 "data: 0.0" 
```

To move Motor 4, *wrist_servo* (y axis), enter:

```
rostopic pub /wrist_servo_controller/command std_msgs/Float64 "data: 0.0"
```

Finally, to move Motor 5, *finger_servo* (y axis), enter:

```
rostopic pub /finger_servo_controller/command std_msgs/Float64 "data: 0.0"
```

## 3. Object detection and Space segmentation:


See the launch file `arm_poses/launch/arm_cam_control.launch` you can edit the parameters for the `object_finder_srv` and `space_finder_srv` services

```
args="-dz_base 0.175 -debug"
```

More specifically, you can remove the `-debug` option and edit the base XYZ offset, where [dx_base, dy_base, dz_base] is the offset of arm's base with respect to the robot's base (ground), where +x to the front, +y to the left, and +z upwards. In particular, **dz_base** is the distance of the arm's base to the plane you want to segment.

Start the arm and cameras by entering:

```
roslaunch arm_poses arm_poses.launch
```

Then, in a separate terminal run

```
roslaunch arm_poses arm_cam_control.launch
```

# Authors

Please, if you use this material, don't forget to add the following reference:

```
@misc{contreras2020,
    title={Arm Control with MoveIt},
    author={Luis Contreras and Hiroyuki Okada},
    url={https://gitlab.com/trcp/armcontroler},
    year={2020}
}
```

* **Luis Contreras** - [AIBot Research Center](http://aibot.jp/)
* **Hiroyuki Okada** - [AIBot Research Center](http://aibot.jp/)
