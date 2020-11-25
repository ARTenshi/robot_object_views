# Multiview Object Generator

## 1. Get the libraries:

### 1.1 Clone this repository

**Warning:** *You only need to do this once. If you have already created this repository in your local machine, pulling it again may cause a loss of your information.*

First, create a workspace:

```
cd ~
mkdir -p ycb_ws/src
cd ycb_ws
catkin_make
```

Then, clone this repository into the src folder:

```
cd ~/ycb_ws/src
git clone https://github.com/ARTenshi/robot_object_views.git
cd ..
catkin_make
source devel/setup.bash
```

**Note**: If you use a different path, update it in the `robotobject/scripts/robot_object_views.py` file.

### 1.2 Test the code

Run the following command in a terminal:

```
roslaunch hsrb_gazebo_launch hsrb_apartment_no_objects_world.launch
```


Then, in another terminal enter these commands:

```
roslaunch robot_object_views eraservisiondebug.launch
```


Finally, in a third terminal, enter:

```
rosrun robot_object_views robot_object_views
```


# Authors

Please, if you use this material, don't forget to add the following reference:

```
@misc{contreras2020,
    title={Multiview Object Generator},
    author={Luis Contreras and Hiroyuki Okada},
    url={https://github.com/ARTenshi/robot_object_views},
    year={2020}
}
```

* **Luis Contreras** - [AIBot Research Center](http://aibot.jp/)
* **Hiroyuki Okada** - [AIBot Research Center](http://aibot.jp/)
